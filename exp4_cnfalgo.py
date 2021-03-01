from pattern_mining import compute_itemsets
from krimp_wrapper import Krimp, db2dat, update_krimp_test_file, get_item_dictionary
from cnfalgo_wrapper import get_cnfalgo_theory, complete_cnfalgo, classify_cnfalgo
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
from time import time
import numpy as np
from pathlib import Path

dataset_class_vars = {
    "iris_17.dat": (17, 18),
    "iris_18.dat": (17, 18),
    "iris_19.dat": (17, 18),
    "zoo.dat": (36, 37),
    "glass.dat": (42, 43),
    "wine.dat": (66, 67),
    "ecoli.dat": (27, 28),
    "hepatitis.dat": (55, 56),
    "heart.dat": (48, 49),
    "dermatology.dat": (44, 45),
    "auto.dat": (132, 133),
    "breast.dat": (19, 20),
    "horseColic.dat": (84, 85),
    "pima.dat": (37, 38),
    "congres.dat": (33, 34),
    "ticTacToe.dat": (28, 29),
    "ionosphere.dat": (156, 157),
    "flare.dat": (31, 32),
    "cylBands.dat": (123, 124),
    "led.dat": (15, 16),
    "soyabean.dat": (100, 101),
    "pageBlocks.dat": (42, 43),
    "nursery.dat": (28, 29),
    "chessKRvK.dat": (41, 42),
}


# topk = 5k-20k
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

# 5k minsup
# krimp_datasets = {
#     "iris_17.dat": "iris_17-all-1d-pop-cccp-20210109105855",
#     "iris_18.dat": "iris_18-all-1d-pop-cccp-20210109105904",
#     "iris_19.dat": "iris_19-all-1d-pop-cccp-20210109105914",
#     "glass.dat": "glass-all-1d-pop-cccp-20210109105923",
#     "wine.dat": "wine-all-3d-pop-cccp-20210109105942",
#     "ecoli.dat": "ecoli-all-1d-pop-cccp-20210109110001",
#     "hepatitis.dat": "hepatitis-all-39d-pop-cccp-20210109110013",
#     "heart.dat": "heart-all-9d-pop-cccp-20210109110032",
#     "dermatology.dat": "dermatology-all-3d-pop-cccp-20210109110103",
#     "auto.dat": "auto-all-24d-pop-cccp-20210109110142",
#     "horseColic.dat": "horseColic-all-23d-pop-cccp-20210109110242",
#     "pima.dat": "pima-all-1d-pop-cccp-20210109110324",
#     "ticTacToe.dat": "ticTacToe-all-7d-pop-cccp-20210109111422",
#     "ionosphere.dat": "ionosphere-all-98d-pop-cccp-20210109110344",
#     "flare.dat": "flare-all-1d-pop-cccp-20210109110449",
#     "cylBands.dat": "cylBands-all-243d-pop-cccp-20210109110719",
#     "led.dat": "led-all-1d-pop-cccp-20210109111353",
# }

# 10k minsup
krimp_datasets = {
    "iris_17.dat": "iris_17-all-1d-pop-cccp-20210117093429",
    "iris_18.dat": "iris_18-all-1d-pop-cccp-20210117093437",
    "iris_19.dat": "iris_19-all-1d-pop-cccp-20210117093444",
    "glass.dat": "glass-all-1d-pop-cccp-20210117093452",
    "wine.dat": "wine-all-3d-pop-cccp-20210117093507",
    "ecoli.dat": "ecoli-all-1d-pop-cccp-20210117093523",
    "hepatitis.dat": "hepatitis-all-39d-pop-cccp-20210117093532",
    "heart.dat": "heart-all-9d-pop-cccp-20210117093549",
    "dermatology.dat": "dermatology-all-3d-pop-cccp-20210117093617",
    "auto.dat": "auto-all-24d-pop-cccp-20210117093651",
    "horseColic.dat": "horseColic-all-23d-pop-cccp-20210117093743",
    "pima.dat": "pima-all-1d-pop-cccp-20210117093833",
    "ticTacToe.dat": "ticTacToe-all-7d-pop-cccp-20210117093848",
    "ionosphere.dat": "ionosphere-all-98d-pop-cccp-20210117094022",
    "flare.dat": "flare-all-1d-pop-cccp-20210117094125",
    "cylBands.dat": "cylBands-all-243d-pop-cccp-20210117094338",
    "led.dat": "led-all-1d-pop-cccp-20210117094950",
}


def get_pct_test_cnfalgo(
    cnfalgo_theory, complete_test_positives, complete_test_negatives, pos_literal=1000
):
    cnfalgo_nb_clauses = len(cnfalgo_theory)

    cnfalgo_nb_literals = 0
    for clause in cnfalgo_theory:
        if pos_literal in clause or -pos_literal in clause:
            cnfalgo_nb_literals += len(clause) - 1
        else:
            cnfalgo_nb_literals += len(clause)

    accuracy = 0

    for c_p in complete_test_positives:
        c_p = list(c_p)
        c_p.append(pos_literal)
        if check_pa_satisfiability(c_p, cnfalgo_theory):
            accuracy += 1

    for c_n in complete_test_negatives:
        c_n = list(c_n)
        c_n.append(-pos_literal)
        if check_pa_satisfiability(c_n, cnfalgo_theory):
            accuracy += 1

    print(
        "CNFAlgo Accuracy = "
        + str(accuracy)
        + "/"
        + str(len(complete_test_positives) + len(complete_test_negatives))
        + " = "
        + str(accuracy / (len(complete_test_positives) + len(complete_test_negatives)))
    )
    accuracy = accuracy / (len(complete_test_positives) + len(complete_test_negatives))
    return cnfalgo_nb_clauses, cnfalgo_nb_literals, accuracy


krimp_exec_path = "./Resources/Krimp/bin/krimp"
output_dir = "./Output/"
seed = 1234
train_pct = 0.8
dl = "me"
version = 6

krimp_accuracy_dict = {}
mistle_accuracy_dict = {}

if len(sys.argv) > 1:
    filename = sys.argv[1]
else:
    filename = None

if len(sys.argv) > 2:
    if sys.argv[1] == "-s":
        suppress_output = True
    else:
        suppress_output = False
else:
    suppress_output = False

for data, class_vars in dataset_class_vars.items():
    if filename is not None:
        if data != filename:
            continue

    pos, neg = load_data(data, class_vars)

    db_file = "Data/" + data
    nb_rows = len(pos) + len(neg)
    rel_minsup = minsup_dict[data]

    print(
        "\n\nComparing on "
        + data
        + " with nb_rows = "
        + str(nb_rows)
        + " and minsup = "
        + str(rel_minsup)
    )

    analysis_result_path = Path("Logs/Exp4_CNFcc_10k_cc/" + data).with_suffix(
        ".db.analysis.txt"
    )
    krimp_item_dict = get_item_dictionary(analysis_result_path)

    cnfalgo_fold_accuracy_list = []
    cnfalgo_time = 0
    for fold in range(1, 11):
        train_file = os.path.join(
            "Logs/Exp4_CNFcc_10k_cc/" + krimp_datasets[data],
            "f" + str(fold),
            "train.db",
        )
        test_file = os.path.join(
            "Logs/Exp4_CNFcc_10k_cc/" + krimp_datasets[data], "f" + str(fold), "test.db"
        )

        train_pos, train_neg = db2dat(train_file, krimp_item_dict, save=True)
        test_pos, test_neg = db2dat(test_file, krimp_item_dict, save=True)

        if len(train_pos) + len(test_pos) != len(pos) or len(train_neg) + len(
            test_neg
        ) != len(neg):
            # This piece of code aligns the positives of Mistle with the positive class considered by KRIMP.
            train_swap = copy(train_pos)
            train_pos = copy(train_neg)
            train_neg = copy(train_swap)

            test_swap = copy(test_pos)
            test_pos = copy(test_neg)
            test_neg = copy(test_swap)
        assert len(train_pos) + len(test_pos) == len(pos)
        assert len(train_neg) + len(test_neg) == len(neg)

        # start_time = time()
        # cnfalgo_theory = get_cnfalgo_theory(train_pos, train_neg)
        # cnfalgo_time += time() - start_time
        #
        # cnfalgo_nb_clauses, cnfalgo_nb_literals, fold_accuracy = get_pct_test_cnfalgo(
        #     cnfalgo_theory, test_pos, test_neg
        # )
        # cnfalgo_fold_accuracy_list.append(fold_accuracy)

        missing_test_positives = []
        for i in range(len(test_pos)):
            missing_test_positives.append(frozenset([]))

        missing_test_negatives = []
        for i in range(len(test_neg)):
            missing_test_negatives.append(frozenset([]))

        nb_clauses, nb_literals, fold_accuracy, cnfalgo_nb_ties = classify_cnfalgo(
            test_pos,
            test_neg,
            test_pos,
            test_neg,
            missing_test_positives,
            missing_test_negatives,
            metric="length",
        )
        cnfalgo_fold_accuracy_list.append(fold_accuracy)

    print("cnfalgo_fold_accuracy_list = " + str(cnfalgo_fold_accuracy_list))
    print("cnfalgo_time = " + str(cnfalgo_time))
    cnfalgo_accuracy = sum(cnfalgo_fold_accuracy_list) / 10
    print("cnfalgo_accuracy = " + str(cnfalgo_accuracy))
