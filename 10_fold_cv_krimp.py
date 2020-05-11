from mistle_v2 import *
from collections import defaultdict
from krimp_wrapper import Krimp
from slim_wrapper import Slim


def compress(pa, definitions):
    """
    :param pa: A partial assignment as a test datapoint
    :param definitions: An ordered dictionary of definitions invented predicates
    :return: A compressed pa that is substituted with some of the invented predicates
    """
    compressed_pa = set(pa)
    new_var_count = 1
    while definitions:
        freq, pattern = definitions.pop()
        if len(pattern) > 1 and pattern.issubset(compressed_pa):
            compressed_pa = compressed_pa - pattern
            compressed_pa.add("z" + str(new_var_count))
            new_var_count += 1

    return compressed_pa


def run_krimp(db_name, min_support, db_suffix=None):
    if db_suffix is None:
        # db_file = "/home/dtai/PycharmProjects/Mistle/Data/" + db_name + ".dat"
        db_file = "Data/" + db_name + ".dat"
    else:
        # db_file = (
        #     "/home/dtai/PycharmProjects/Mistle/Output/"
        #     + db_name
        #     + "/"
        #     + db_name
        #     + "_"
        #     + db_suffix
        #     + ".dat"
        # )
        db_file = "Output/" + db_name + "/" + db_name + "_" + db_suffix + ".dat"

    krimp_exec_path = "Resources/Krimp/bin/krimp"
    output_dir = "Output/"
    code_table = Krimp(krimp_exec_path).compute_code_table(
        db_file, output_dir, min_support=min_support, convert_db=True
    )

    # print(code_table)
    return code_table


def run_slim(db_name, min_support, db_suffix=None):
    if db_suffix is None:
        # db_file = "/home/dtai/PycharmProjects/Mistle/Data/" + db_name + ".dat"
        db_file = "Data/" + db_name + ".dat"
    else:
        # db_file = (
        #     "/home/dtai/PycharmProjects/Mistle/Output/"
        #     + db_name
        #     + "/"
        #     + db_name
        #     + "_"
        #     + db_suffix
        #     + ".dat"
        # )
        db_file = "Output/" + db_name + "/" + db_name + "_" + db_suffix + ".dat"

    slim_exec_path = "Resources/Slim/bin/fic"
    output_dir = "Output/"
    code_table = Slim(slim_exec_path).compute_code_table(
        db_file, output_dir, min_support=min_support, convert_db=True
    )

    # print(code_table)
    return code_table


def test_both_theories_by_compression(
    pos_def, neg_def, test_positives, test_negatives, default_prediction
):
    accuracy = 0

    TP_c = 0
    TN_c = 0
    FP_c = 0
    FN_c = 0

    TP_f = 0
    TN_f = 0
    FP_f = 0
    FN_f = 0

    # Update negative literals
    pos_def_2 = []
    for freq, pattern in pos_def:
        new_pattern = set()
        for literal in pattern:
            if literal > 32768:
                new_pattern.add(literal - 65536)
            else:
                new_pattern.add(literal)
        pos_def_2.append((freq, new_pattern))

    neg_def_2 = []
    for freq, pattern in neg_def:
        new_pattern = set()
        for literal in pattern:
            if literal > 32768:
                new_pattern.add(literal - 65536)
            else:
                new_pattern.add(literal)
        neg_def_2.append((freq, new_pattern))

    for pa in test_positives:
        pos_length = len(compress(pa, copy(pos_def_2)))
        neg_length = len(compress(pa, copy(neg_def_2)))

        if pos_length < neg_length:
            # Correctly classified as a positive
            accuracy += 1
            TP_c += 1
        elif neg_length < pos_length:
            # Wrongly classified as a negative
            FN_c += 1
        else:
            if default_prediction == "+":
                accuracy += 1
                TP_f += 1
            elif default_prediction == "-":
                FN_f += 1

    for pa in test_negatives:
        pos_length = len(compress(pa, copy(pos_def_2)))
        neg_length = len(compress(pa, copy(neg_def_2)))

        if pos_length < neg_length:
            # Wrongly classified as a positive
            FP_c += 1
        elif neg_length < pos_length:
            # Correctly classified as a negative
            accuracy += 1
            TN_c += 1
        else:
            if default_prediction == "+":
                TP_f += 1
            elif default_prediction == "-":
                accuracy += 1
                FN_f += 1

    print(
        "Confusion Matrix \t: TP_c = "
        + str(TP_c)
        + "; TN_c = "
        + str(TN_c)
        + "; FP_c = "
        + str(FP_c)
        + "; FN_c = "
        + str(FN_c)
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
            "TP_c": TP_c,
            "TN_c": TN_c,
            "FP_c": FP_c,
            "FN_c": FN_c,
            "TP_f": TP_f,
            "TN_f": TN_f,
            "FP_f": FP_f,
            "FN_f": FN_f,
        },
    )


def cross_validate(filename, num_folds=10, minsup=(25, 50), system="krimp"):
    start_time = time()
    avg_accuracy = 0.0
    confusions = defaultdict(int)

    for fold in range(num_folds):
        pos_def = None
        neg_def = None
        if system == "krimp":
            pos_def = run_krimp(filename, minsup[0], str(fold) + "_train_pos")
            neg_def = run_krimp(filename, minsup[1], str(fold) + "_train_neg")
        elif system == "slim":
            pos_def = run_slim(filename, minsup[0], str(fold) + "_train_pos")
            neg_def = run_slim(filename, minsup[1], str(fold) + "_train_neg")

        with open(
            "./Output/"
            + filename
            + "/"
            + filename
            + "_"
            + str(fold)
            + "_train_pos.dat",
            "r",
        ) as f:
            lines = f.readlines()
            nb_positives = len(lines)

        with open(
            "./Output/"
            + filename
            + "/"
            + filename
            + "_"
            + str(fold)
            + "_train_neg.dat",
            "r",
        ) as f:
            lines = f.readlines()
            nb_negatives = len(lines)

        if nb_positives > nb_negatives:
            default_prediction = "+"
        else:
            default_prediction = "-"

        test_positives = []
        with open(
            "./Output/" + filename + "/" + filename + "_" + str(fold) + "_test_pos.dat",
            "r",
        ) as f:
            lines = f.readlines()
            for line in lines:
                line = line.strip().split(" ")
                test_positives.append([int(item) for item in line])

        test_negatives = []
        with open(
            "./Output/" + filename + "/" + filename + "_" + str(fold) + "_test_neg.dat",
            "r",
        ) as f:
            lines = f.readlines()
            for line in lines:
                line = line.strip().split(" ")
                test_negatives.append([int(item) for item in line])

        (fold_accuracy, fold_confusions) = test_both_theories_by_compression(
            pos_def, neg_def, test_positives, test_negatives, default_prediction
        )

        print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy))

        for k, v in fold_confusions.items():
            confusions[k] += v

    tp = 0.0
    tn = 0.0
    fp = 0.0
    fn = 0.0
    for k, v in confusions.items():
        if "TP" in k:
            tp += v
        elif "TN" in k:
            tn += v
        elif "FP" in k:
            fp += v
        elif "FN" in k:
            fn += v

    precision = 0 if tp + fp == 0 else tp / (tp + fp)
    recall = 0 if tp + fn == 0 else tp / (tp + fn)
    accuracy = 0 if tp + tn + fp + fn == 0 else (tp + tn) / (tp + tn + fp + fn)

    print("\nTotal Confusions\t\t\t\t\t: " + str(confusions))
    print("Precision\t\t\t\t\t\t\t: " + str(round(precision * 100, 2)) + "%")
    print("Recall\t\t\t\t\t\t\t\t: " + str(round(recall * 100, 2)) + "%")
    print("Accuracy\t\t\t\t\t\t\t: " + str(round(accuracy * 100, 2)) + "%")

    avg_accuracy = avg_accuracy / num_folds

    print(
        "Average Classification Accuracy \t: " + str(round(avg_accuracy * 100, 2)) + "%"
    )
    print("Total Time \t\t\t\t\t\t\t: " + str(time() - start_time) + " seconds")
    return avg_accuracy


if __name__ == "__main__":
    filename = sys.argv[1]
    pos_minsup = int(sys.argv[2])
    neg_minsup = int(sys.argv[3])
    system = sys.argv[4]

    cross_validate(
        filename, num_folds=10, minsup=(pos_minsup, neg_minsup), system=system
    )
