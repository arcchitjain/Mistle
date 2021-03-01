import matplotlib
import matplotlib.pyplot as plt

plt.style.use("seaborn")
matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=26)
matplotlib.rc("axes", titlesize=26)
matplotlib.rc("axes", labelsize=26)
matplotlib.rc("xtick", labelsize=26)
matplotlib.rc("ytick", labelsize=26)
matplotlib.rc("legend", fontsize=26)
matplotlib.rc("figure", titlesize=26)
lylim = 0
uylim = 1.0
version = 10

fig = plt.figure()
ax1 = fig.add_axes([0.15, 0.15, 0.8, 0.8])
ax1.set_ylabel("Completion Accuracy 1")
ax1.set_ylim(bottom=lylim, top=uylim)
ax1.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax1.set_yticklabels(["0.0", "0.2", "0.4", "0.6", "0.8", "1.0"])
ax1.set_xticks([0.5, 0.6, 0.7, 0.8, 0.9])
ax1.set_xticklabels(["0.5", "0.6", "0.7", "0.8", "0.9"])
ax1.set_xlabel("Partial Obvservability Parameter")

po_parameters = [0.9, 0.8, 0.7, 0.6, 0.5]
mistle_accuracy_list = [
    0.4575484246274099,
    0.3067038951580178,
    0.1761600930892958,
    0.0946598954285173,
    0.046581495907257034,
]
# mistle_clauses_list = [38.5, 38.5, 38.5, 38.5, 38.5]
# mistle_literals_list = [225.8, 225.8, 225.8, 225.8, 225.8]
cnfalgo_accuracy_list = [
    0.39942352006282517,
    0.3526399447641536,
    0.23788644917720828,
    0.11374246268460957,
    0.05186113067518916,
]
# cnfalgo_clauses_list = [6916.4, 6916.4, 6916.4, 6916.4, 6916.4]
# cnfalgo_literals_list = [34028.0, 34028.0, 34028.0, 34028.0, 34028.0]
# train_data_literals_list = [10346.4, 10346.4, 10346.4, 10346.4, 10346.4]
randomized_accuracy_list = [
    0.3494154720258318,
    0.23018823352345055,
    0.13326191157653297,
    0.06517439229514303,
    0.031845899928442535,
]
ax1.plot(po_parameters, mistle_accuracy_list, marker="o", label="Mistle")
ax1.plot(po_parameters, cnfalgo_accuracy_list, marker="o", label="CNF-cc")
ax1.plot(po_parameters, randomized_accuracy_list, marker="o", label="Random")
ax1.legend()
plt.savefig("exp2_plot1_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()

fig = plt.figure(figsize=(10, 7))
ax2 = fig.add_axes([0.125, 0.15, 0.6, 0.8])
ax2.set_ylabel("Completion Accuracy 2")
ax2.set_ylim(bottom=lylim, top=uylim)
ax2.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax2.set_yticklabels(["0.0", "0.2", "0.4", "0.6", "0.8", "1.0"])
ax2.set_xticks([0.5, 0.6, 0.7, 0.8, 0.9])
ax2.set_xticklabels(["0.5", "0.6", "0.7", "0.8", "0.9"])
ax2.set_xlabel("Partial Obvservability Parameter")

po_parameters = [0.9, 0.8, 0.7, 0.6, 0.5]
mistle_accuracy_list1 = [
    0.7131123396479725,
    0.6417820410320259,
    0.634364404787649,
    0.6070103123512263,
    0.6358006847893657,
]
mistle_accuracy_list2 = [
    0.6566931869568483,
    0.5659245453083045,
    0.48214150653394805,
    0.40035810301975133,
    0.37318775179134056,
]
mistle_accuracy_list3 = [
    0.7083985600104221,
    0.6947009283186425,
    0.7096297159211238,
    0.7420750008995574,
    0.7576925217842754,
]
mistle_accuracy_list4 = [
    0.6519794073192979,
    0.6188434325949213,
    0.5574068176674227,
    0.5354227915680825,
    0.4950795887862502,
]
cnfalgo_accuracy_list = [
    0.5144429128901429,
    0.4034604312693341,
    0.3035038638314072,
    0.22273699148413484,
    0.1892233525360675,
]
randomized_accuracy_list = [
    0.35676026507964764,
    0.2318938852666804,
    0.13733124834369984,
    0.06748979068397692,
    0.028722446861102684,
]

ax2.plot(po_parameters, mistle_accuracy_list1, marker="o", label="M1")
ax2.plot(po_parameters, mistle_accuracy_list2, marker="o", label="M2")
ax2.plot(po_parameters, mistle_accuracy_list3, marker="o", label="M3")
ax2.plot(po_parameters, mistle_accuracy_list4, marker="o", label="M4")
ax2.plot(po_parameters, cnfalgo_accuracy_list, marker="o", label="CNF-cc")
ax2.plot(po_parameters, randomized_accuracy_list, marker="o", label="Random")
ax2.legend(bbox_to_anchor=(1, 1), loc="upper left")
plt.savefig("exp2_plot2_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()


fig = plt.figure(figsize=(7, 7))
ax3 = fig.add_axes([0.15, 0.15, 0.8, 0.8])
ax3.set_ylabel("Completion Accuracy 3")
ax3.set_ylim(bottom=lylim, top=uylim)
ax3.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax3.set_yticklabels(["0.0", "0.2", "0.4", "0.6", "0.8", "1.0"])

mistle_accuracy1 = 0.7420315225852726
mistle_accuracy2 = 0.8703735849953027
mistle_accuracy3 = 0.641900001717907
mistle_accuracy4 = 0.7702420641279369
# mistle_clauses = 38.5
# mistle_literals = 225.8
cnfalgo_accuracy = 0.8806884650113839
# cnfalgo_clauses = 6916.4
# cnfalgo_literals = 30101.2
# train_data_literals = 10346.4

ax3.bar(
    ["M1", "M2", "M3", "M4", "CNF-cc"],
    [
        mistle_accuracy1,
        mistle_accuracy2,
        mistle_accuracy3,
        mistle_accuracy4,
        cnfalgo_accuracy,
    ],
)
plt.savefig("exp2_plot3_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()


fig = plt.figure(figsize=(14, 7))
ax4 = fig.add_axes([0.125, 0.15, 0.4, 0.7])
ax4.set_ylabel("Completion Accuracy 4")
ax4.set_ylim(bottom=lylim, top=uylim)
ax4.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax4.set_yticklabels(["0.0", "0.2", "0.4", "0.6", "0.8", "1.0"])
ax4.set_title("Dataset: TicTacToe")

num_folds = 10

mistle_accuracy_list1 = [
    0.970873786407767,
    0.8902439024390244,
    0.872093023255814,
    0.8762886597938144,
    0.9444444444444444,
    0.8602150537634409,
    0.88,
    0.8255813953488372,
    0.8631578947368421,
    0.8888888888888888,
]
mistle_accuracy_list2 = [
    0.9805825242718447,
    0.9878048780487805,
    0.9534883720930233,
    0.979381443298969,
    1.0,
    0.967741935483871,
    0.99,
    0.9534883720930233,
    0.9473684210526315,
    0.9814814814814815,
]
mistle_accuracy_list3 = [
    0.9902912621359223,
    0.9024390243902439,
    0.9186046511627907,
    0.8969072164948454,
    0.9444444444444444,
    0.8924731182795699,
    0.89,
    0.872093023255814,
    0.9157894736842105,
    0.9074074074074074,
]
mistle_accuracy_list4 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
mistle_clauses_list = [20, 45, 43, 41, 36, 36, 33, 33, 36, 39]
mistle_literals_list = [70, 166, 163, 153, 131, 135, 123, 134, 138, 146]
cnfalgo_accuracy_list = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
cnfalgo_clauses_list = [
    22700,
    22860,
    22405,
    23338,
    23640,
    23093,
    22693,
    22395,
    23123,
    23022,
]
cnfalgo_literals_list = [
    103467,
    104200,
    102091,
    106431,
    107960,
    105294,
    103426,
    102132,
    105434,
    105074,
]
train_data_literals_list = [7695, 7884, 7848, 7749, 7650, 7785, 7722, 7848, 7767, 7650]

mistle_clauses = sum(mistle_clauses_list) / num_folds
mistle_literals = sum(mistle_literals_list) / num_folds

mistle_accuracy1 = sum(mistle_accuracy_list1) / num_folds
mistle_accuracy2 = sum(mistle_accuracy_list2) / num_folds
mistle_accuracy3 = sum(mistle_accuracy_list3) / num_folds
mistle_accuracy4 = sum(mistle_accuracy_list4) / num_folds

cnfalgo_clauses = sum(cnfalgo_clauses_list) / num_folds
cnfalgo_literals = sum(cnfalgo_literals_list) / num_folds
cnfalgo_accuracy = sum(cnfalgo_accuracy_list) / num_folds

ax4.bar(
    ["M1", "M2", "M3", "M4", "CNF-cc"],
    [
        mistle_accuracy1,
        mistle_accuracy2,
        mistle_accuracy3,
        mistle_accuracy4,
        cnfalgo_accuracy,
    ],
)

ax5 = fig.add_axes([0.55, 0.15, 0.4, 0.7])
ax5.set_ylim(bottom=lylim, top=uylim)
ax5.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax5.set_yticklabels([])
ax5.set_title("Dataset: PIMA")

num_folds = 10
mistle_accuracy_list1 = [
    0.3037974683544304,
    0.3230769230769231,
    0.375,
    0.3466666666666667,
    0.34146341463414637,
    0.4050632911392405,
    0.37349397590361444,
    0.3088235294117647,
    0.6046511627906976,
    0.3563218390804598,
]
mistle_accuracy_list2 = [
    0.8987341772151899,
    0.8307692307692308,
    0.96875,
    0.9066666666666666,
    0.926829268292683,
    0.9367088607594937,
    0.8433734939759037,
    0.9264705882352942,
    0.6511627906976745,
    0.8160919540229885,
]
mistle_accuracy_list3 = [
    0.1518987341772152,
    0.2153846153846154,
    0.125,
    0.10666666666666667,
    0.18292682926829268,
    0.12658227848101267,
    0.26506024096385544,
    0.11764705882352941,
    0.627906976744186,
    0.25287356321839083,
]
mistle_accuracy_list4 = [
    0.7468354430379747,
    0.7230769230769231,
    0.71875,
    0.6666666666666666,
    0.7682926829268293,
    0.6582278481012658,
    0.7349397590361446,
    0.7352941176470589,
    0.6744186046511628,
    0.7126436781609196,
]
mistle_clauses_list = [32, 42, 34, 41, 31, 33, 38, 40, 42, 41]
mistle_literals_list = [135, 181, 138, 175, 132, 133, 163, 168, 154, 169]
cnfalgo_accuracy_list = [
    0.8354430379746836,
    0.8153846153846154,
    0.90625,
    0.9066666666666666,
    0.8536585365853658,
    0.8987341772151899,
    0.7951807228915663,
    0.9117647058823529,
    0.7441860465116279,
    0.8390804597701149,
]
cnfalgo_clauses_list = [4874, 5493, 5737, 5990, 5751, 5662, 5469, 5251, 5001, 5713]
cnfalgo_literals_list = [
    17116,
    19700,
    20655,
    21691,
    20810,
    20332,
    19707,
    18744,
    17566,
    20504,
]
train_data_literals_list = [5512, 5624, 5632, 5544, 5488, 5512, 5480, 5600, 5456, 5448]

mistle_clauses = sum(mistle_clauses_list) / num_folds
mistle_literals = sum(mistle_literals_list) / num_folds

mistle_accuracy1 = sum(mistle_accuracy_list1) / num_folds
mistle_accuracy2 = sum(mistle_accuracy_list2) / num_folds
mistle_accuracy3 = sum(mistle_accuracy_list3) / num_folds
mistle_accuracy4 = sum(mistle_accuracy_list4) / num_folds

cnfalgo_clauses = sum(cnfalgo_clauses_list) / num_folds
cnfalgo_literals = sum(cnfalgo_literals_list) / num_folds
cnfalgo_accuracy = sum(cnfalgo_accuracy_list) / num_folds

ax5.bar(
    ["M1", "M2", "M3", "M4", "CNF-cc"],
    [
        mistle_accuracy1,
        mistle_accuracy2,
        mistle_accuracy3,
        mistle_accuracy4,
        cnfalgo_accuracy,
    ],
)

plt.savefig("exp2_plot4_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()
