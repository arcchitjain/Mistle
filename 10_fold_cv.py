from mistle_class import *
import random
from tqdm import tqdm
from pycosat import itersolve


def count_solutions(pa, theory):

    theory_cnf = [tuple(clause) for clause in theory]

    cnf = theory_cnf + [(a,) for a in pa]

    return len(list(itersolve(cnf)))


def split_data(data, num_folds=10, seed=1234):
    random.seed(seed)

    split_data = [[] for i in range(num_folds)]

    for datapoint in data:
        split_data[int(random.random() * num_folds)].append(datapoint)

    return split_data


def test_theory(theory, positives, negatives):
    accuracy = 0
    total_datapoints = len(positives) + len(negatives)

    pbar = tqdm(total=total_datapoints)
    pbar.set_description("Testing theory")

    for pa in positives:
        if check_pa_satisfiability(pa, theory):
            accuracy += 1
        pbar.update(1)

    for pa in negatives:
        if not check_pa_satisfiability(pa, theory):
            accuracy += 1
        pbar.update(1)

    pbar.close()

    return float(accuracy) / total_datapoints


# def test_both_theories(pos_theory, neg_theory, positives, negatives):
#     accuracy = 0
#     total_datapoints = len(positives) + len(negatives)
#
#     pbar = tqdm(total=total_datapoints)
#     pbar.set_description("Testing both theories")
#
#     for pa in positives:
#         if count_solutions(pa, pos_theory) > count_solutions(pa, neg_theory):
#             accuracy += 1
#         elif count_solutions(pa, pos_theory) == count_solutions(pa, neg_theory):
#             accuracy += 0.5
#         pbar.update(1)
#
#     for pa in negatives:
#         if count_solutions(pa, pos_theory) < count_solutions(pa, neg_theory):
#             accuracy += 1
#         elif count_solutions(pa, pos_theory) == count_solutions(pa, neg_theory):
#             accuracy += 0.5
#         pbar.update(1)
#
#     pbar.close()
#
#     return float(accuracy) / total_datapoints


def cross_validate(
    positives, negatives, num_folds=10, output_file=None, lossless=False
):

    avg_accuracy = 0.0
    avg_compression = 0.0
    seed = 1234
    total_datapoints = len(positives) + len(negatives)

    split_positives = split_data(positives, num_folds, seed=seed)
    split_negatives = split_data(negatives, num_folds, seed=seed)

    for fold in range(num_folds):
        train_positives = []
        train_negatives = []
        test_positives = []
        test_negatives = []

        for i in range(num_folds):

            if i == fold:
                test_positives = split_positives[i]
                test_negatives = split_negatives[i]
            else:
                train_positives += split_positives[i]
                train_negatives += split_negatives[i]

        if output_file is not None:
            pbar = tqdm(total=total_datapoints)
            pbar.set_description("Outputting data")

            train_file = open(output_file + "_train_" + str(fold) + ".db", "w+")

            for pa in train_positives + train_negatives:
                l = list(pa)
                abs_l = [abs(i) for i in l]
                train_file.write(
                    " ".join([str(x) for _, x in sorted(zip(abs_l, l))]) + "\n"
                )
                pbar.update(1)

            train_file.close()

            test_file = open(output_file + "_test_" + str(fold) + ".db", "w+")

            for pa in test_positives + test_negatives:
                l = list(pa)
                abs_l = [abs(i) for i in l]
                test_file.write(
                    " ".join([str(x) for _, x in sorted(zip(abs_l, l))]) + "\n"
                )
                pbar.update(1)

            test_file.close()
            pbar.close()

        # Compressed Theory
        mistle = Mistle(train_positives, train_negatives)
        theory, compression = mistle.learn(lossless=lossless)
        # theory = mistle.convert_to_theory(train_negatives)
        # compression = 0
        # pos_theory = Mistle(train_positives, train_negatives).learn(lossless=lossless)
        # neg_theory = Mistle(train_negatives, train_positives).learn(lossless=lossless)

        # Uncompressed Theory
        # mistle = Mistle(train_positives, train_negatives)
        # theory = mistle.convert_to_theory(train_negatives)

        fold_accuracy = test_theory(theory, test_positives, test_negatives)
        # fold_accuracy = test_both_theories(
        #     pos_theory, neg_theory, test_positives, test_negatives
        # )
        print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy))
        avg_accuracy += fold_accuracy
        avg_compression += compression

    avg_accuracy = avg_accuracy / num_folds
    avg_compression = avg_compression / num_folds
    print(
        "Average Classification Accuracy \t: " + str(round(avg_accuracy * 100, 2)) + "%"
    )
    print("Average Compression \t: " + str(avg_compression) + "%")

    return avg_accuracy


# positives, negatives = load_ionosphere()
# positives, negatives = load_chess(switch_signs=True)
positives, negatives = load_mushroom()
# cross_validate(positives, negatives, 10, "./Output/adult", lossless=False)
cross_validate(positives, negatives, 10, "./Output/mushroom", lossless=False)
# cross_validate(positives, negatives, 10, lossless=False)
