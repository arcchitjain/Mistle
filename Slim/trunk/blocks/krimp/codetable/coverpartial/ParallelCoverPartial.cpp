#include "../global.h"
#include <time.h>
#include <omp.h>

#include <db/Database.h>
#include <itemstructs/ItemSet.h>
#include <isc/ItemSetCollection.h>

#include "../CodeTable.h"

#include "CoverPartial.h"
#include "ParallelCoverPartial.h"

ParallelCoverPartial::ParallelCoverPartial(CodeTable *ct) : CoverPartial(ct) {
}

enum MessageType { MsgAdd, MsgPrune };

struct Message {
	MessageType type;
	uint64 timeStamp;
	ItemSet* itemSet;

	Message() {
	}

	Message(MessageType type, uint64 timeStamp, ItemSet* itemSet)
		: type(type), timeStamp(timeStamp), itemSet(itemSet) {
	}
};	

enum StateType { Init, Cover, Prune, CommitAdd, CommitDel, UpdateCT, UpdatePruneList, Done };

struct ThreadState {
	StateType state;
	uint64 timeStamp;
	uint64 curM;
	islist::iterator pruneIter;
	islist* pruneList;
	uint32 pruneListIndex;
};


CodeTable* ParallelCoverPartial::DoeJeDing(const uint64 candidateOffset, const uint32 startSup) {
	mCompressionStartTime = omp_get_wtime();
	uint64 numM = mISC->GetNumItemSets(), nextReportCnd;
	int32 nextReportSup;
	mProgress = -1;

	if(mWriteReportFile == true) 
		OpenReportFile(true);
	if(mWriteCTLogFile == true) {
		OpenCTLogFile();
		mCT->SetCTLogFile(mCTLogFile);
	}

	mStartTime = time(NULL);
	mScreenReportTime = time(NULL);
	mScreenReportCandidateIdx = 0;
	mScreenReportCandPerSecond = 0;
	mScreenReportCandidateDelta = 5000;

	CoverStats stats = mCT->GetCurStats();
	printf(" * Start:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);

	nextReportSup = (mReportSupDelta == 0) ? 0 : startSup - mReportSupDelta;
	nextReportCnd = (mReportCndDelta == 0) ? numM : mReportCndDelta;

	uint32 numThreads = omp_get_max_threads();


	deque<ItemSet*> candidatesBuffer;
	uint64 candidatesBufferOffset = candidateOffset;
	deque<islist*> pruneListBuffer;
	uint32 pruneListBufferOffset = 0;

	ThreadState global;
	global.state = Cover;
	global.curM = candidateOffset;
	global.timeStamp = 0;

	uint32 numRechecked = 0;
	uint64 timeStampTotal = 0;

	vector<ThreadState> local(numThreads);
	vector<deque<Message> > messages(numThreads);
	vector<CodeTable*> localCT(numThreads);
	for(uint32 i=0; i<numThreads; i++) {
		local[i].state = Init;
		localCT[i] = i == 0 ? mCT : mCT->Clone();
	}

    #pragma omp parallel
	{
		uint32 tid = omp_get_thread_num();
		StateType& state = local[tid].state;
		CodeTable* ct = localCT[tid];
		uint64& timeStamp = local[tid].timeStamp;
		uint64& curM = local[tid].curM;
		uint32& pruneListIndex = local[tid].pruneListIndex;
		islist::iterator& pruneIter = local[tid].pruneIter;

		deque<Message> processedMessages;
		uint64 minTimeStamp;
		bool pruneNotUsed;
		ItemSet *m = NULL;
		Message msg;

		while (state != Done) {
			#pragma omp critical
			{
				do {
					if (state == Cover) {
						if (m->GetLength() > 1) {
							bool abort = !messages[tid].empty() && messages[tid].front().timeStamp < timeStamp;
							if(!abort && ct->GetCurSize() < ct->GetPrevSize()) {
								mCT = ct;
								global = local[tid];
								global.curM++;
								global.timeStamp++;
								if(mPruneStrategy == PreAcceptPruneStrategy) {
									throw string("Pre-pruning not supported.");
								} else if (mPruneStrategy == PostAcceptPruneStrategy) {
									global.state = UpdatePruneList;
									global.pruneList = NULL;
								}

								state = CommitAdd;

								// Vertel de andere threads dat ze deze kandidaat moeten toevoegen.
								for(uint32 i=0; i<numThreads; i++) {
									if(local[i].state != Done) {
										// Kandidaten die na de huidige kandidaat komen zijn niet geldig en
										// moeten dus niet meer toegevoegd worden
										while (!messages[i].empty() && timeStamp < messages[i].back().timeStamp) {
											messages[i].back().itemSet->UnRef();
											messages[i].pop_back();
										}
									}
									if (i != tid) {
										m->Ref();
										messages[i].push_back(Message(MsgAdd, timeStamp, m));
									}
								}

								if(mReportOnAccept == true) {
									// we moeten alle accepts rapporteren
									ProgressToDisk(mCT, m->GetSupport(), 0 /* TODO: curLength */, curM, false, true);
								} 

								for(uint32 i=0; i<numThreads; i++) {
									if(i != tid && (local[i].state == Cover || local[i].state == Prune) && local[i].timeStamp < timeStamp) {
										m->Ref();
										processedMessages.push_back(Message(MsgAdd, timeStamp, m));
										break;
									}
								}
							} else {
								ct->RollbackAdd();
							}
						}

					} else if (state == Prune) {
						bool abort = !messages[tid].empty() && messages[tid].front().timeStamp < timeStamp;

						if (!abort && (pruneNotUsed || ct->GetCurSize() <= ct->GetPrevSize())) {
							mCT = ct;
							global = local[tid];
							global.timeStamp++;

							if (pruneNotUsed) {
								ct->Del(m, true, false); // zap immediately

								global.pruneIter++;
								if (global.pruneIter == global.pruneList->end()) {
									global.state = Cover;
								}
							} else {
								state = CommitDel;
								global.state = UpdatePruneList;
							}
							
							// Vertel de andere threads dat ze de itemset moeten verwijderen.
							for(uint32 i=0; i<numThreads; i++) {
								if(local[i].state != Done) {
									while (!messages[i].empty() && timeStamp < messages[i].back().timeStamp) {
										messages[i].back().itemSet->UnRef();
										messages[i].pop_back();
									}
								}
								if (i != tid) {
									m->Ref();
									messages[i].push_back(Message(MsgPrune, timeStamp, m));
								}
							}

							for(uint32 i=0; i<numThreads; i++) {
								if(i != tid && (local[i].state == Cover || local[i].state == Prune) && local[i].timeStamp < timeStamp) {
									m->Ref();
									processedMessages.push_back(Message(MsgPrune, timeStamp, m));
									break;
								}
							}
						} else if (!pruneNotUsed) {
							ct->RollbackLastDel();
						}
						m->UnRef();

					} else if (state == CommitAdd) {
						state = Cover;
					} else if (state == CommitDel) {
						state = Prune;
					}

					if (state != CommitAdd && state != CommitDel) {
						if (!messages[tid].empty()) {
							state = UpdateCT;
							msg = messages[tid].front();
							messages[tid].pop_front();

							minTimeStamp = !messages[tid].empty() ? messages[tid].front().timeStamp : global.timeStamp;
							for (uint32 i=0; i<numThreads; i++) {
								if (i != tid && (local[i].state == Cover || local[i].state == Prune) && local[i].timeStamp < minTimeStamp) {
									minTimeStamp = local[i].timeStamp;
								}
							}
						} else {
							if (global.state == UpdatePruneList) {
								if (global.pruneList == NULL) {
									global.pruneList = ct->GetPostPruneList();

									if (global.pruneList->empty()) {
										delete global.pruneList;
										global.state = Cover;
									} else {
										global.state = Prune;
										global.pruneIter = global.pruneList->begin();
										pruneListBuffer.push_back(global.pruneList);
										global.pruneListIndex = pruneListBufferOffset + (uint32) pruneListBuffer.size() - 1;
									}
								} else {
									global.state = Prune;
									// Zijn er nog andere threads bezig met de huidige pruneList?
									for(uint32 i=0; i<numThreads; i++) {
										if(i != tid && local[i].state == Prune && local[i].timeStamp < global.timeStamp && local[i].pruneListIndex == global.pruneListIndex) {
											// Ja, maak een kopie van de rest van de pruneList
											global.pruneList = new islist(global.pruneIter, global.pruneList->end());
											global.pruneIter = global.pruneList->begin();
											pruneListBuffer.push_back(global.pruneList);
											global.pruneListIndex = pruneListBufferOffset + (uint32) pruneListBuffer.size() - 1;
											break;
										}
									}
									ct->UpdatePostPruneList(global.pruneList, global.pruneIter);
									global.pruneIter++;

									if (global.pruneIter == global.pruneList->end()) {
										global.state = Cover;
									}
								}
							}

							if (global.state == Cover) {
								local[tid] = global;
								if(global.curM < numM) {
									if(global.curM < candidatesBufferOffset + candidatesBuffer.size()) {
										m = candidatesBuffer[(uint32) (global.curM - candidatesBufferOffset)];
										numRechecked++;
									} else {
										m = mISC->GetNextItemSet();
										m->SetID(global.curM);
										candidatesBuffer.push_back(m);
									}
									m->Ref();
									global.curM++;
									global.timeStamp++;
									timeStampTotal++;
								} else {
									state = Done;
								}
							} else if (global.state == Prune) {
								local[tid] = global;
								m = *global.pruneIter;
								m->Ref();
								pruneNotUsed = ct->GetUsageCount(m) == 0;

								global.pruneIter++;
								global.timeStamp++;
								timeStampTotal++;
								if (global.pruneIter == global.pruneList->end()) {
									global.state = Cover;
									global.pruneListIndex = UINT32_MAX_VALUE;
								}
							}

							// Verwijder kandidaten en pruneLists die niet meer gerechecked hoeven te worden
							uint64 candidatesMin = curM;
							size_t pruneListMin = state == Prune ? pruneListIndex : pruneListBufferOffset + pruneListBuffer.size();
							for(uint32 i=0; i<numThreads; i++) {
								if(i != tid && local[i].timeStamp < timeStamp) {
									if ((local[i].state == Cover || local[i].state == Prune) && local[i].curM < candidatesMin) {
										candidatesMin = local[i].curM;
									}
									if (local[i].state == Prune && local[i].pruneListIndex < pruneListMin) {
										pruneListMin = local[i].pruneListIndex;
									}
								}
							}

							while (candidatesBufferOffset < candidatesMin) {
								candidatesBuffer.front()->UnRef();
								candidatesBuffer.pop_front();
								candidatesBufferOffset++;
							}

							while (pruneListBufferOffset < pruneListMin) {
								delete pruneListBuffer.front();
								pruneListBuffer.pop_front();
								pruneListBufferOffset++;
							}
						}
					}
				} while (state == Prune && pruneNotUsed);
			}
			if(state == Done) {
				for(uint32 i=0; i<processedMessages.size(); i++) {
					processedMessages[i].itemSet->UnRef();
				}
			} else if (state == Cover) {
				int32 curSup = m->GetSupport();
				CoverStats &curStats = ct->GetCurStats();

				#pragma omp master
				{
					ProgressToScreen(curM, numM);

					// Er kan een codetable worden geschreven die niet geldig is, omdat een andere
					// thread een kandidaat kan toevoegen die eerder komt dan de huidige kandidaat.
					if(mWriteProgressToDisk) {
						if(curM == 0) {
							ProgressToDisk(mCT, curSup, 0 /* TODO: curLength */, curM, true, true);
							// !!! nextReportSup moet niet 0-based, maar minSup-based zijn (dus (curSup - minSup) % mRepSupD == 0, ofzo
							nextReportSup = (mReportSupDelta == 0) ? 0 : curSup - (((curSup-mISC->GetMinSupport()) % mReportSupDelta) == 0 ? mReportSupDelta : (curSup-mISC->GetMinSupport()) % mReportSupDelta);
						} else if(curSup < nextReportSup) {		// curSup < nRS, dus we moeten volledig reporten!
							ProgressToDisk(mCT, nextReportSup, 0 /* TODO: curLength */, curM, true, true);
							nextReportSup -= mReportSupDelta;
							if(mReportCndDelta != 0)
								nextReportCnd = curM + mReportCndDelta;
							if(nextReportSup < 1)
								nextReportSup = 1;
							while(nextReportSup > curSup) {
								ProgressToDisk(mCT, nextReportSup, 0 /* TODO: curLength */, curM, false, true);
								nextReportSup -= mReportSupDelta;
							}
						} else if(curM == nextReportCnd) {
							ProgressToDisk(mCT, nextReportSup+mReportSupDelta, 0 /* TODO: curLength */, curM, true, false);
							nextReportCnd += mReportCndDelta;
						}
					}
				}

				if(m->GetLength() <= 1)	{	// Skip singleton Itemsets: already in alphabet
					m->UnRef();
				} else {
					ct->Add(m, m->GetID());
					ct->CoverDB(curStats);

					if(curStats.encDbSize < 0) {
						THROW("dbSize < 0. Dit is niet goed.");
					}

				}


			} else if (state == Prune) {
				ct->Del(m, false, false);
				ct->CoverDB(ct->GetCurStats());
			} else if (state == CommitAdd) {
				ct->CommitAdd();
			} else if (state == CommitDel) {
				ct->CommitLastDel(true);
			} else if (state == UpdateCT) {
				while(!processedMessages.empty() && processedMessages.back().timeStamp > msg.timeStamp) {
					Message& msg = processedMessages.back();
					if (msg.type == MsgAdd) {
						ct->DelAndCommit(msg.itemSet, true);
						msg.itemSet->UnRef();
					} else if (msg.type == MsgPrune) {
						ct->UndoPrune(msg.itemSet);
					}
					processedMessages.pop_back();
				}

				if (msg.type == MsgAdd) {
					msg.itemSet->Ref();
					ct->AddAndCommit(msg.itemSet, msg.itemSet->GetID());
				} else if (msg.type == MsgPrune) {
					if (ct->GetUsageCount(msg.itemSet) == 0) {
						ct->Del(msg.itemSet, true, false); // zap immediately
					} else {
						ct->DelAndCommit(msg.itemSet, true);
					}
				}

				// Verwijder messages die niet meer ongedaan kunnen worden
				while(!processedMessages.empty() && processedMessages.front().timeStamp < minTimeStamp) {
					processedMessages.front().itemSet->UnRef();
					processedMessages.pop_front();
				}

				if (msg.timeStamp < minTimeStamp) {
					msg.itemSet->UnRef();
				} else {
					processedMessages.push_back(msg);
				}
			}
		}
	} // end parallel section
	if(mPruneStrategy == SanitizeOfflinePruneStrategy) {				// Sanitize post-pruning
		PruneSanitize(mCT);
	}

	for(uint32 i=0; i<numThreads; i++) {
		if (localCT[i] != mCT) {
			delete localCT[i];
		}
	}

	printf(" * Parallel:\t\t%d candidates had to be rechecked.\n", numRechecked);
	double timeCompression = omp_get_wtime() - mCompressionStartTime;
	printf(" * Time:\t\tCompressing the database took %f seconds.\n", timeCompression);
	stats = mCT->GetCurStats();
	printf(" * Result:\t\t(%da,%du,%" I64d ",%.0lf,%.0lf,%.0lf)\n",stats.alphItemsUsed, stats.numSetsUsed, stats.usgCountSum,stats.encDbSize,stats.encCTSize,stats.encSize);
	if(mWriteProgressToDisk) {
		ProgressToDisk(mCT, mISC->GetMinSupport(), 0 /* TODO: curLength */, numM, true, true);
	}
	CloseCTLogFile();
	CloseReportFile();


//	printf("%f%% of candidates were rechecked. (%" I64d "/%" I64d ")\n", ((double) timeStampTotal / global.timeStamp  - 1) * 100, timeStampTotal - global.timeStamp, global.timeStamp);

	return mCT;
}
