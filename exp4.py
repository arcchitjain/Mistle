from pattern_mining import compute_itemsets
from krimp_wrapper import Krimp, db2dat, update_krimp_test_file
from mistle_v2 import (
    Mistle,
    check_pa_satisfiability,
    get_clauses,
    load_data,
    get_topk_minsup,
)
import os
import sys
import random
from copy import copy


def split_data(data, train_pct=0.7, seed=0):
    random.seed(seed)

    train_data = []
    test_data = []

    for datapoint in data:
        if random.random() < train_pct:
            train_data.append(datapoint)
        else:
            test_data.append(datapoint)

    return train_data, test_data

    # dataset_class_vars = {
    #     "iris_17.dat": (17, 18),
    #     "iris_18.dat": (17, 18),
    #     "iris_19.dat": (17, 18),
    #     "zoo.dat": (36, 37),
    #     "glass.dat": (42, 43),
    #     "wine.dat": (66, 67),
    #     "ecoli.dat": (27, 28),
    #     "hepatitis.dat": (55, 56),
    #     "heart.dat": (48, 49),
    #     "dermatology.dat": (44, 45),
    #     "auto.dat": (132, 133),
    #     "breast.dat": (19, 20),
    #     "horseColic.dat": (84, 85),
    #     "pima.dat": (37, 38),
    #     "congres.dat": (33, 34),
    #     "ticTacToe.dat": (28, 29),
    #     "ionosphere.dat": (156, 157),
    #     "flare.dat": (31, 32),
    #     "cylBands.dat": (123, 124),
    #     "led.dat": (15, 16),
    #     "soyabean.dat": (100, 101),
    # }


dataset_class_vars = {
    "pageBlocks.dat": (42, 43),
    "nursery.dat": (28, 29),
    "chessKRvK.dat": (41, 42),
}


# topk = 10k
minsup_dict = {
    "iris_17.dat": 0.006666666666666667,
    "iris_18.dat": 0.006666666666666667,
    "iris_19.dat": 0.006666666666666667,
    "zoo.dat": 0.009900990099009901,
    "glass.dat": 0.004672897196261682,
    "wine.dat": 0.015625,
    "ecoli.dat": 0.002976190476190476,
    "hepatitis.dat": 0.25,
    "heart.dat": 0.03125,
    "dermatology.dat": 0.0078125,
    "auto.dat": 0.11640381366943867,
    "breast.dat": 0.001430615164520744,
    "horseColic.dat": 0.06149123818307586,
    "pima.dat": 0.0013020833333333333,
    "congres.dat": 0.1995470492747552,
    "ticTacToe.dat": 0.0078125,
    "ionosphere.dat": 0.27960369179497835,
    "flare.dat": 0.0007199424046076314,
    "cylBands.dat": 0.45031445120414193,
    "led.dat": 0.0003125,
    "soyabean.dat": 0.49355773072313225,
}

# int_minsup_dict = {
#     "iris_17.dat": 1,
#     "iris_18.dat": 1,
#     "iris_19.dat": 1,
#     "zoo.dat": 1,
#     "glass.dat": 1,
#     "wine.dat": 3,
#     "ecoli.dat": 1,
#     "hepatitis.dat": 39,
#     "heart.dat": 9,
#     "dermatology.dat": 3,
#     "auto.dat": 24,
#     "breast.dat": 1,
#     "horseColic.dat": 23,
#     "pima.dat": 1,
#     "congres.dat": 87,
#     "ticTacToe.dat": 7,
#     "ionosphere.dat": 98,
#     "flare.dat": 1,
#     "cylBands.dat": 243,
#     "led.dat": 1,
#     "soyabean.dat": 337,
# }

minsup_dict = {}
int_minsup_dict = {}
for data, class_vars in dataset_class_vars.items():
    name = data.split(".")[0]
    print(
        "\n\n####################################### "
        + name
        + " #######################################"
    )
    pos, neg = load_data(data, class_vars)
    r = get_topk_minsup(pos + neg, suppress_output=True)
    int_minsup = round(r * (len(pos) + len(neg)))
    print("Optimal Minsup for " + str(name) + ": " + str(r))
    minsup_dict[data] = r
    int_minsup_dict[data] = int_minsup

print("minsup_dict = " + str(minsup_dict))
print("int_minsup_dict = " + str(int_minsup_dict))


# def classify_by_satisfiability(pos_theory, neg_theory, test_pos, test_neg):
#     # pos_theory = T+ := \neg(V p)
#     # neg_theory = T- := \neg(V n)
#     # if UNSAT(p, T+) and SAT(p, T-):
#     #     "Classify as Positive"
#     # elif SAT(p, T+) and UNSAT(p, T-):
#     #     "Classify as Negative"
#
#     mistle_accuracy = 0
#     attempted_test_pos = []
#     attempted_test_neg = []
#
#     pos_clauses = get_clauses(pos_theory)
#     neg_clauses = get_clauses(neg_theory)
#
#     for pos in test_pos:
#         if not check_pa_satisfiability(pos, pos_clauses) and check_pa_satisfiability(
#             pos, neg_clauses
#         ):
#             mistle_accuracy += 1
#             attempted_test_pos.append(pos)
#         elif check_pa_satisfiability(pos, pos_clauses) and not check_pa_satisfiability(
#             pos, neg_clauses
#         ):
#             attempted_test_pos.append(pos)
#
#     for neg in test_neg:
#         if not check_pa_satisfiability(neg, pos_clauses) and check_pa_satisfiability(
#             neg, neg_clauses
#         ):
#             attempted_test_neg.append(neg)
#         elif check_pa_satisfiability(neg, pos_clauses) and not check_pa_satisfiability(
#             neg, neg_clauses
#         ):
#             mistle_accuracy += 1
#             attempted_test_neg.append(neg)
#
#     if len(attempted_test_pos) + len(attempted_test_neg) > 0:
#         mistle_accuracy = mistle_accuracy / (
#             len(attempted_test_pos) + len(attempted_test_neg)
#         )
#     else:
#         mistle_accuracy = None
#     return mistle_accuracy, attempted_test_pos, attempted_test_neg
#
#
# krimp_exec_path = "./Resources/Krimp/bin/krimp"
# output_dir = "./Output/"
# seed = 0
# train_pct = 0.7
# dl = "me"
# version = 2
#
# krimp_accuracy_dict = {}
# mistle_accuracy_dict = {}
#
# if len(sys.argv) > 1:
#     filename = sys.argv[1]
# else:
#     filename = None
#
# if len(sys.argv) > 2:
#     if sys.argv[1] == "-s":
#         suppress_output = True
#     else:
#         suppress_output = False
# else:
#     suppress_output = False
#
# for data, class_vars in dataset_class_vars.items():
#     if filename is not None:
#         if data != filename:
#             continue
#
#     pos, neg = load_data(data, class_vars)
#
#     # train_pos, test_pos = split_data(pos, train_pct, seed)
#     # train_neg, test_neg = split_data(neg, train_pct, seed)
#
#     db_file = "Data/" + data
#     nb_rows = len(pos) + len(neg)
#     rel_minsup = minsup_dict[data]
#
#     print(
#         "\n\nComparing on "
#         + data
#         + " with nb_rows = "
#         + str(nb_rows)
#         + " and minsup = "
#         + str(rel_minsup)
#     )
#
#     # res_path, krimp_item_dict = Krimp(krimp_exec_path).classifycompress(
#     #     db_file,
#     #     output_dir,
#     #     class_vars=class_vars,
#     #     min_support=max(round(rel_minsup * nb_rows), 1),
#     #     convert_db=True,
#     #     seed=seed,
#     #     suppress_output=suppress_output,
#     # )
#     # print("res_path = " + str(res_path))
#     # print("krimp_item_dict = " + str(krimp_item_dict))
#
#     res_path = "./Output/xps/classify/congres-all-87d-pop-cccp-20200905171732/"
#     krimp_item_dict = {
#         0: 11,
#         1: 31,
#         2: 33,
#         3: 22,
#         4: 5,
#         5: 27,
#         6: 8,
#         7: 15,
#         8: 13,
#         9: 2,
#         10: 30,
#         11: 24,
#         12: 19,
#         13: 20,
#         14: 9,
#         15: 25,
#         16: 10,
#         17: 17,
#         18: 18,
#         19: 26,
#         20: 3,
#         21: 4,
#         22: 1,
#         23: 14,
#         24: 16,
#         25: 7,
#         26: 29,
#         27: 23,
#         28: 6,
#         29: 28,
#         30: 34,
#         31: 12,
#         32: 21,
#         33: 32,
#     }
#
#     mistle_fold_accuracy_list = []
#     mistle_atempted = 0
#
#     for fold in range(1, 11):
#         train_file = os.path.join(res_path, "f" + str(fold), "train.db")
#         test_file = os.path.join(res_path, "f" + str(fold), "test.db")
#
#         train_pos, train_neg = db2dat(train_file, krimp_item_dict, save=True)
#         test_pos, test_neg = db2dat(test_file, krimp_item_dict, save=True)
#
#         if len(train_pos) + len(test_pos) != len(pos) or len(train_neg) + len(
#             test_neg
#         ) != len(neg):
#             train_swap = copy(train_pos)
#             train_pos = copy(train_neg)
#             train_neg = copy(train_swap)
#
#             test_swap = copy(test_pos)
#             test_pos = copy(test_neg)
#             test_neg = copy(test_swap)
#         assert len(train_pos) + len(test_pos) == len(pos)
#         assert len(train_neg) + len(test_neg) == len(neg)
#
#         mistle_pos_theory, _ = Mistle(train_neg, train_pos).learn(
#             minsup=max(round(rel_minsup * len(train_pos)), 1)
#         )
#         mistle_neg_theory, _ = Mistle(train_pos, train_neg).learn(
#             minsup=max(round(rel_minsup * len(train_neg)), 1)
#         )
#
#         fold_accuracy, attempted_test_pos, attempted_test_neg = classify_by_satisfiability(
#             mistle_pos_theory, mistle_neg_theory, test_pos, test_neg
#         )
#         if fold_accuracy is not None:
#             mistle_fold_accuracy_list.append(fold_accuracy)
#
#         update_krimp_test_file(
#             res_path, fold, attempted_test_pos, attempted_test_neg, krimp_item_dict
#         )
#
#         mistle_atempted += len(attempted_test_pos) + len(attempted_test_neg)
#
#     krimp_accuracy, krimp_std_dev, best_minsup = Krimp(
#         krimp_exec_path
#     ).get_krimp_accuracy(res_path)
#     krimp_accuracy_dict[data] = krimp_accuracy
#     print("krimp_accuracy = " + str(krimp_accuracy))
#
#     mistle_accuracy = sum(mistle_fold_accuracy_list) / len(mistle_fold_accuracy_list)
#
#     print("mistle_accuracy = " + str(mistle_accuracy))
#     print("mistle_atempted = " + str(mistle_atempted))
#     print("mistle_coverage = " + str(mistle_atempted / nb_rows))
#
#     mistle_accuracy_dict[data] = (
#         mistle_accuracy,
#         mistle_atempted,
#         mistle_atempted / nb_rows,
#     )
#
#
# print("krimp_accuracy_dict = " + str(krimp_accuracy_dict))
# print("mistle_accuracy_dict = " + str(mistle_accuracy_dict))
