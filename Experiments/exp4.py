from krimp_wrapper import Krimp
import os
import statistics
from mistle_v2 import *


def translate(krimp_vars, krimp_item_dict, sort=False):
    if isinstance(krimp_vars, list):

        translated_vars = []
        for krimp_var in krimp_vars:
            literal = int(krimp_item_dict[int(krimp_var)])
            if literal > 32768:
                literal = literal - 65536
            translated_vars.append(literal)

        if sort:
            abs_translated_vars = [abs(i) for i in translated_vars]
            return [x for _, x in sorted(zip(abs_translated_vars, translated_vars))]
        else:
            return translated_vars
    else:
        literal = int(krimp_item_dict[int(krimp_vars)])
        if literal > 32768:
            return str(literal - 65536)
        else:
            return str(literal)


def db2dat(file, krimp_item_dict, save=True):
    """
    The lower class label is by default called the negative class and the older label is the positive class.
    :param file: file path to a ".db" which is to be read and converted to ".dat" format with original var names
    :param krimp_item_dict: A mapping of krimp var names to the original var names
    :param save: If True, then the ".dat" version of the file is saved
    :return:
    """
    positives = []
    negatives = []

    with open(file, "r") as f:
        lines = f.readlines()

        assert len(lines) > 5
        class_line = lines[5].strip()
        krimp_labels = class_line[4:].split(" ")
        assert len(krimp_labels) == 2

        if save:
            pos_file = file[:-3] + "_pos.dat"
            neg_file = file[:-3] + "_neg.dat"

            pos_f = open(pos_file, "w+")
            neg_f = open(neg_file, "w+")

        for datapoint in lines[6:]:
            krimp_vars = datapoint.strip().split(" ")[1:]
            datapoint_pa = krimp_vars[:-1]
            datapoint_class = krimp_vars[-1]
            if save:
                original_vars = translate(datapoint_pa, krimp_item_dict, sort=True)
            else:
                original_vars = translate(datapoint_pa, krimp_item_dict, sort=False)

            if krimp_labels[0] in datapoint_class:
                negatives.append(frozenset(original_vars))
                if save:
                    pos_f.write(
                        (" ".join([str(literal) for literal in original_vars])) + "\n"
                    )

            elif krimp_labels[1] in datapoint_class:
                positives.append(frozenset(original_vars))
                if save:
                    neg_f.write(
                        (" ".join([str(literal) for literal in original_vars])) + "\n"
                    )

            else:
                print("Cannot assign label to " + " ".join(original_vars))

        if save:
            pos_f.close()
            neg_f.close()

    return positives, negatives


def classify_by_satisfiability(
    pos_theory, neg_theory, test_positives, test_negatives, default_prediction=None
):
    accuracy = 0
    TP_s, TN_s, FP_s, FN_s = 0, 0, 0, 0
    TP_f, TN_f, FP_f, FN_f = 0, 0, 0, 0

    p_clauses = get_clauses(pos_theory)
    n_clauses = get_clauses(neg_theory)

    for pa in test_positives:
        p = check_pa_satisfiability(pa, p_clauses)
        n = check_pa_satisfiability(pa, n_clauses)

        if (
            (p is False and n is True)
            or (p is None and n is True)
            or (p is False and n is None)
        ):
            # Correctly classified as a positive
            accuracy += 1
            TP_s += 1
        elif (
            (p is True and n is False)
            or (p is None and n is False)
            or (p is True and n is None)
        ):
            # Wrongly classified as a negative
            FN_s += 1
        else:
            if default_prediction == "+":
                accuracy += 1
                TP_f += 1
            elif default_prediction == "-":
                FN_f += 1

    for pa in test_negatives:
        p = check_pa_satisfiability(pa, p_clauses)
        n = check_pa_satisfiability(pa, n_clauses)

        if (
            (p is False and n is True)
            or (p is None and n is True)
            or (p is False and n is None)
        ):
            # Wrongly classified as a positive
            FP_s += 1
        elif (
            (p is True and n is False)
            or (p is None and n is False)
            or (p is True and n is None)
        ):
            # Correctly classified as a negative
            accuracy += 1
            TN_s += 1
        else:
            if default_prediction == "+":
                FP_f += 1
            elif default_prediction == "-":
                accuracy += 1
                TN_f += 1

    print(
        "Confusions \t: TP_s = "
        + str(TP_s)
        + "; TN_s = "
        + str(TN_s)
        + "; FP_s = "
        + str(FP_s)
        + "; FN_s = "
        + str(FN_s)
        + "; TP_f = "
        + str(TP_f)
        + "; TN_f = "
        + str(TN_f)
        + "; FP_f = "
        + str(FP_f)
        + "; FN_f = "
        + str(FN_f)
    )

    return (
        float(accuracy) / (len(test_positives) + len(test_negatives)),
        {
            "TP_s": TP_s,
            "TN_s": TN_s,
            "FP_s": FP_s,
            "FN_s": FN_s,
            "TP_f": TP_f,
            "TN_f": TN_f,
            "FP_f": FP_f,
            "FN_f": FN_f,
        },
    )


def count_failed_clauses(pa, clauses):
    if len(clauses) == 0:
        return 0

    nb_failed_clauses = 0
    for clause in clauses:
        if solve([tuple(clause)] + [(a,) for a in pa]) == "UNSAT":
            nb_failed_clauses += 1
    return nb_failed_clauses / len(clauses)


def classify_by_failing_clauses(
    pos_theory, neg_theory, test_positives, test_negatives, default_prediction=None
):
    accuracy = 0
    TP_s, TN_s, FP_s, FN_s = 0, 0, 0, 0
    TP_f, TN_f, FP_f, FN_f = 0, 0, 0, 0

    p_clauses = get_clauses(pos_theory)
    n_clauses = get_clauses(neg_theory)

    for pa in test_positives:
        p = count_failed_clauses(pa, p_clauses)
        n = count_failed_clauses(pa, n_clauses)

        if p > n:
            # Correctly classified as a positive
            accuracy += 1
            TP_s += 1
        elif p < n:
            # Wrongly classified as a negative
            FN_s += 1
        else:
            if default_prediction == "+":
                accuracy += 1
                TP_f += 1
            elif default_prediction == "-":
                FN_f += 1

    for pa in test_negatives:
        p = count_failed_clauses(pa, p_clauses)
        n = count_failed_clauses(pa, n_clauses)

        if p > n:
            # Wrongly classified as a positive
            FP_s += 1
        elif p < n:
            # Correctly classified as a negative
            accuracy += 1
            TN_s += 1
        else:
            if default_prediction == "+":
                FP_f += 1
            elif default_prediction == "-":
                accuracy += 1
                TN_f += 1

    print(
        "Confusions \t: TP_s = "
        + str(TP_s)
        + "; TN_s = "
        + str(TN_s)
        + "; FP_s = "
        + str(FP_s)
        + "; FN_s = "
        + str(FN_s)
        + "; TP_f = "
        + str(TP_f)
        + "; TN_f = "
        + str(TN_f)
        + "; FP_f = "
        + str(FP_f)
        + "; FN_f = "
        + str(FN_f)
    )

    return (
        float(accuracy) / (len(test_positives) + len(test_negatives)),
        {
            "TP_s": TP_s,
            "TN_s": TN_s,
            "FP_s": FP_s,
            "FN_s": FN_s,
            "TP_f": TP_f,
            "TN_f": TN_f,
            "FP_f": FP_f,
            "FN_f": FN_f,
        },
    )


# minsup = 1
# db_file = "/home/dtai/PycharmProjects/Mistle/Data/breast.dat"
# class_vars = [19, 20]
# minsup = 600
# db_file = "/home/dtai/PycharmProjects/Mistle/Data/chess.dat"
# class_vars = [74, 75]
minsup = 50
db_file = "/home/dtai/PycharmProjects/Mistle/Data/ticTacToe.dat"
class_vars = [28, 29]
os.chdir("..")
krimp_exec_path = "Resources/Krimp/bin/krimp"
output_dir = "Output/"
seed = 1234
dl = "me"

# accuracy, std_dev, best_minsup, res_path, krimp_item_dict = Krimp(
#     krimp_exec_path
# ).classify(
#     db_file,
#     output_dir,
#     class_vars=class_vars,
#     min_support=minsup,
#     convert_db=True,
#     seed=seed,
# )

res_path = "/home/dtai/PycharmProjects/Mistle/Output/xps/classify/ticTacToe-all-50d-pop-cccp-20200511213650"
krimp_item_dict = {
    0: 28,
    1: 13,
    2: 25,
    3: 19,
    4: 7,
    5: 1,
    6: 22,
    7: 16,
    8: 10,
    9: 4,
    10: 14,
    11: 26,
    12: 20,
    13: 8,
    14: 2,
    15: 29,
    16: 23,
    17: 17,
    18: 11,
    19: 5,
    20: 24,
    21: 18,
    22: 12,
    23: 6,
    24: 27,
    25: 21,
    26: 9,
    27: 3,
    28: 15,
}
# best_minsup = int(best_minsup) / 100
# print("KRIMP accuracy", accuracy)
# print("KRIMP std_dev", std_dev)
# print("KRIMP minsup", best_minsup)

mistle_accuracy_list = []

for fold in range(1, 11):
    train_file = os.path.join(res_path, "f" + str(fold), "train.db")
    test_file = os.path.join(res_path, "f" + str(fold), "test.db")

    train_pos, train_neg = db2dat(train_file, krimp_item_dict, save=True)
    test_pos, test_neg = db2dat(test_file, krimp_item_dict, save=True)

    mistle_pos_theory, _ = Mistle(train_neg, train_pos).learn(
        dl_measure=dl, minsup=minsup, lossy=True, prune=True
    )
    mistle_neg_theory, _ = Mistle(train_pos, train_neg).learn(
        dl_measure=dl, minsup=minsup, lossy=True, prune=True
    )

    if len(train_pos) > len(train_neg):
        default_prediction = "+"
    else:
        default_prediction = "-"

    fold_accuracy, fold_confusions = classify_by_failing_clauses(
        mistle_pos_theory, mistle_neg_theory, test_pos, test_neg, default_prediction
    )

    mistle_accuracy_list.append(fold_accuracy)
    print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy))


mistle_avg_accuracy = statistics.mean(mistle_accuracy_list)
mistle_std_dev_accuracy = statistics.stdev(mistle_accuracy_list)

print("MISTLE accuracy " + str(mistle_avg_accuracy))
print("MISTLE std_dev " + str(mistle_std_dev_accuracy))
