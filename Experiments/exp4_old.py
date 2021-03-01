from krimp_wrapper import Krimp
import os
import statistics
from mistle_v2 import *
import matplotlib
import matplotlib.pyplot as plt

# import mplcyberpunk

plt.style.use("seaborn")
matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=24)
matplotlib.rc("axes", titlesize=22)
matplotlib.rc("axes", labelsize=22)
matplotlib.rc("xtick", labelsize=22)
matplotlib.rc("ytick", labelsize=22)
matplotlib.rc("legend", fontsize=22)
matplotlib.rc("figure", titlesize=22)


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


def save_complete_dataset(dataset):
    """
    Apply closed world assumption on the data to complete it
    :param dataset: Name of the dataset in string
    :return: a tuple of positives and negatives (which are complete, i.e., they contain every variables in each row)
    """
    positives, negatives = globals()["load_" + dataset]()
    complete_positives = []
    complete_negatives = []
    if dataset == "breast":
        var_range = list(range(1, 19))
        class_vars = [19, 20]
    elif dataset == "tictactoe":
        var_range = list(range(1, 28))
        class_vars = [28, 29]
    else:
        print("Invalid Dataset Name")
        return None

    for positive_pa in positives:
        complete_pa = set(positive_pa)
        for var in var_range:
            if var not in positive_pa:
                complete_pa.add(-var)
        complete_positives.append(complete_pa)

    for negative_pa in negatives:
        complete_pa = set(negative_pa)
        for var in var_range:
            if var not in negative_pa:
                complete_pa.add(-var)
        complete_negatives.append(complete_pa)

    output_filename = os.path.abspath("Data/" + dataset + "_completed.dat")
    with open(output_filename, "w+") as f:
        for pa in complete_positives:
            f.write(
                " ".join([str(literal) for literal in pa])
                + " "
                + str(class_vars[1])
                + "\n"
            )
        for pa in complete_negatives:
            f.write(
                " ".join([str(literal) for literal in pa])
                + " "
                + str(class_vars[0])
                + "\n"
            )

    return complete_positives, complete_negatives, output_filename


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


def count_passed_clauses(pa, clauses):
    if len(clauses) == 0:
        return 0

    nb_passed_clauses = 0
    for clause in clauses:
        if solve([tuple(clause)] + [(a,) for a in pa]) != "UNSAT":
            nb_passed_clauses += 1
    return nb_passed_clauses / len(clauses)


def classify_by_passing_clauses(
    pos_theory, neg_theory, test_positives, test_negatives, default_prediction=None
):
    accuracy = 0
    TP_s, TN_s, FP_s, FN_s = 0, 0, 0, 0
    TP_f, TN_f, FP_f, FN_f = 0, 0, 0, 0

    p_clauses = get_clauses(pos_theory)
    n_clauses = get_clauses(neg_theory)

    for pa in test_positives:
        p = count_passed_clauses(pa, n_clauses)
        n = count_passed_clauses(pa, p_clauses)

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
        p = count_passed_clauses(pa, n_clauses)
        n = count_passed_clauses(pa, p_clauses)

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


def classify_by_shortest_theory(
    pos_theory, neg_theory, test_positives, test_negatives, default_prediction=None
):
    accuracy = 0
    TP_s, TN_s, FP_s, FN_s = 0, 0, 0, 0
    TP_f, TN_f, FP_f, FN_f = 0, 0, 0, 0

    p_clauses = get_clauses(pos_theory)
    n_clauses = get_clauses(neg_theory)

    if neg_theory is None and pos_theory is None:
        use = "d"
    elif neg_theory is None:
        use = "p"
    elif pos_theory is None:
        use = "n"
    elif pos_theory.dl < neg_theory.dl:
        use = "p"
    elif neg_theory.dl < pos_theory.dl:
        use = "n"
    elif neg_theory.dl == pos_theory.dl:
        use = "d"
    else:
        use = "d"

    for pa in test_positives:
        if use == "d":
            if default_prediction == "+":
                accuracy += 1
                TP_f += 1
            elif default_prediction == "-":
                FN_f += 1
        elif use == "p":
            if check_pa_satisfiability(pa, p_clauses):
                # Correctly classified as a positive
                accuracy += 1
                TP_s += 1
            else:
                # Wrongly classified as a negative
                FN_s += 1
        elif use == "n":
            if check_pa_satisfiability(pa, n_clauses):
                # Wrongly classified as a negative
                FN_s += 1
            else:
                # Correctly classified as a positive
                accuracy += 1
                TP_s += 1

    for pa in test_negatives:
        if use == "d":
            if default_prediction == "+":
                FP_f += 1
            elif default_prediction == "-":
                accuracy += 1
                TN_f += 1
        elif use == "p":
            if check_pa_satisfiability(pa, p_clauses):
                # Wrongly classified as a positive
                FP_s += 1
            else:
                # Correctly classified as a negative
                accuracy += 1
                TN_s += 1
        elif use == "n":
            if check_pa_satisfiability(pa, n_clauses):
                # Wrongly classified as a positive
                FP_s += 1
            else:
                # Correctly classified as a negative
                accuracy += 1
                TN_s += 1

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


def check_pa_satisfiability_on_errors(pa, errors):
    if errors is None or len(errors) == 0:
        return None
    else:
        for error in errors:
            if solve([(l,) for l in error] + [(a,) for a in pa]) != "UNSAT":
                return True
        return False


def classify_by_shortest_theory_with_errors(
    pos_theory, neg_theory, test_positives, test_negatives, default_prediction=None
):
    accuracy = 0
    TP_s, TN_s, FP_s, FN_s = 0, 0, 0, 0
    TP_f, TN_f, FP_f, FN_f = 0, 0, 0, 0

    p_clauses = get_clauses(pos_theory)
    n_clauses = get_clauses(neg_theory)
    p_errors = get_errors(pos_theory)
    n_errors = get_errors(neg_theory)

    if neg_theory is None and pos_theory is None:
        use = "d"
    elif neg_theory is None:
        use = "p"
    elif pos_theory is None:
        use = "n"
    elif pos_theory.dl < neg_theory.dl:
        use = "p"
    elif neg_theory.dl < pos_theory.dl:
        use = "n"
    elif neg_theory.dl == pos_theory.dl:
        use = "d"
    else:
        use = "d"

    for pa in test_positives:

        if use == "d":
            if default_prediction == "+":
                accuracy += 1
                TP_f += 1
            elif default_prediction == "-":
                FN_f += 1
        elif use == "p":
            p_t = check_pa_satisfiability(pa, p_clauses)
            p_e = check_pa_satisfiability_on_errors(pa, p_errors)
            if (p_t is True and p_e is True) or (p_t is False and p_e is False):
                # Correctly classified as a positive
                accuracy += 1
                TP_s += 1
            elif (p_t is False and p_e is True) or (p_t is True and p_e is False):
                # Wrongly classified as a negative
                FN_s += 1
        elif use == "n":
            n_t = check_pa_satisfiability(pa, n_clauses)
            n_e = check_pa_satisfiability_on_errors(pa, n_errors)
            if (n_t is True and n_e is True) or (n_t is False and n_e is False):
                # Wrongly classified as a negative
                FN_s += 1
            elif (n_t is False and n_e is True) or (n_t is True and n_e is False):
                # Correctly classified as a positive
                accuracy += 1
                TP_s += 1

    for pa in test_negatives:

        if use == "d":
            if default_prediction == "+":
                FP_f += 1
            elif default_prediction == "-":
                accuracy += 1
                TN_f += 1
        elif use == "p":
            p_t = check_pa_satisfiability(pa, p_clauses)
            p_e = check_pa_satisfiability_on_errors(pa, p_errors)
            if (p_t is True and p_e is True) or (p_t is False and p_e is False):
                # Wrongly classified as a positive
                FP_s += 1
            elif (p_t is False and p_e is True) or (p_t is True and p_e is False):
                # Correctly classified as a negative
                accuracy += 1
                TN_s += 1
        elif use == "n":
            n_t = check_pa_satisfiability(pa, n_clauses)
            n_e = check_pa_satisfiability_on_errors(pa, n_errors)
            if (n_t is True and n_e is True) or (n_t is False and n_e is False):
                # Wrongly classified as a positive
                FP_s += 1
            elif (n_t is False and n_e is True) or (n_t is True and n_e is False):
                # Correctly classified as a negative
                accuracy += 1
                TN_s += 1

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


start_time = time()
os.chdir("..")
krimp_exec_path = "Resources/Krimp/bin/krimp"
output_dir = "Output/"
seed = 1234
dl = "me"
version = 2

# minsup = 1
# db_file = "/home/dtai/PycharmProjects/Mistle/Data/breast.dat"
# class_vars = [19, 20]
# minsup = 600
# db_file = "/home/dtai/PycharmProjects/Mistle/Data/chess.dat"
# class_vars = [74, 75]

# cp, cn, db_file = save_complete_dataset("tictactoe")
# minsup = int(0.2 * (len(cp) + len(cn)))

rel_minsups = [0.6, 0.7, 0.8]
krimp_accuracy_list = []
krimp_std_dev_list = []
mistle1_accuracy_list = []
mistle1_std_dev_list = []
mistle2_accuracy_list = []
mistle2_std_dev_list = []
mistle3_accuracy_list = []
mistle3_std_dev_list = []
mistle4_accuracy_list = []
mistle4_std_dev_list = []
mistle5_accuracy_list = []
mistle5_std_dev_list = []

# dataset = "tictactoe"
# dataset = "breast"
# dataset = "pima"
dataset = "breast"

for rel_minsup in rel_minsups:

    # Tictactoe
    # db_file = "/home/dtai/PycharmProjects/Mistle/Data/tictactoe_completed.dat"
    # nb_rows = 958
    # class_vars = [28, 29]

    # Breast
    class_vars = [19, 20]
    nb_rows = 699

    # PIMA
    # class_vars = [37, 38]
    # nb_rows = 768

    # Ionosphere
    # class_vars = [156, 157]
    # nb_rows = 351

    db_file = "/home/arcchitjain/Documents/Mistle/Data/" + dataset + ".dat"
    minsup = int(rel_minsup * nb_rows)

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
    #
    # krimp_accuracy_list.append(float(accuracy))
    # krimp_std_dev_list.append(float(std_dev))
    # print("KRIMP accuracy", accuracy)
    # print("KRIMP std_dev", std_dev)

    mistle1_fold_accuracy_list = []
    mistle2_fold_accuracy_list = []
    mistle3_fold_accuracy_list = []
    mistle4_fold_accuracy_list = []
    mistle5_fold_accuracy_list = []

    for fold in range(1, 2):
        train_file = os.path.join(res_path, "f" + str(fold), "train.db")
        test_file = os.path.join(res_path, "f" + str(fold), "test.db")

        train_pos, train_neg = db2dat(train_file, krimp_item_dict, save=True)
        test_pos, test_neg = db2dat(test_file, krimp_item_dict, save=True)

        mistle_pos_theory, _ = Mistle(train_neg, train_pos).learn(
            dl_measure=dl,
            minsup=int(rel_minsup * len(train_pos)),
            lossy=True,
            prune=True,
        )
        mistle_neg_theory, _ = Mistle(train_pos, train_neg).learn(
            dl_measure=dl,
            minsup=int(rel_minsup * len(train_neg)),
            lossy=True,
            prune=True,
        )

        if len(train_pos) > len(train_neg):
            default_prediction = "+"
        else:
            default_prediction = "-"

        fold_accuracy1, _ = classify_by_satisfiability(
            mistle_pos_theory, mistle_neg_theory, test_pos, test_neg, default_prediction
        )

        mistle1_fold_accuracy_list.append(fold_accuracy1)

        fold_accuracy2, _ = classify_by_failing_clauses(
            mistle_pos_theory, mistle_neg_theory, test_pos, test_neg, default_prediction
        )

        mistle2_fold_accuracy_list.append(fold_accuracy2)

        fold_accuracy3, _ = classify_by_passing_clauses(
            mistle_pos_theory, mistle_neg_theory, test_pos, test_neg, default_prediction
        )

        mistle3_fold_accuracy_list.append(fold_accuracy3)

        fold_accuracy4, _ = classify_by_shortest_theory(
            mistle_pos_theory, mistle_neg_theory, test_pos, test_neg, default_prediction
        )

        mistle4_fold_accuracy_list.append(fold_accuracy4)

        fold_accuracy5, _ = classify_by_shortest_theory_with_errors(
            mistle_pos_theory, mistle_neg_theory, test_pos, test_neg, default_prediction
        )

        mistle5_fold_accuracy_list.append(fold_accuracy5)

    mistle1_accuracy_list.append(statistics.mean(mistle1_fold_accuracy_list))
    # mistle1_std_dev_list.append(statistics.stdev(mistle1_fold_accuracy_list))

    mistle2_accuracy_list.append(statistics.mean(mistle2_fold_accuracy_list))
    # mistle2_std_dev_list.append(statistics.stdev(mistle2_fold_accuracy_list))

    mistle3_accuracy_list.append(statistics.mean(mistle3_fold_accuracy_list))
    # mistle3_std_dev_list.append(statistics.stdev(mistle3_fold_accuracy_list))

    mistle4_accuracy_list.append(statistics.mean(mistle4_fold_accuracy_list))
    # mistle4_std_dev_list.append(statistics.stdev(mistle4_fold_accuracy_list))

    mistle5_accuracy_list.append(statistics.mean(mistle5_fold_accuracy_list))
    # mistle5_std_dev_list.append(statistics.stdev(mistle5_fold_accuracy_list))

    print(
        "Relative Minsup "
        + str(rel_minsup)
        + "% : Mistle1: "
        + str(statistics.mean(mistle1_fold_accuracy_list))
    )
    print(
        "Relative Minsup "
        + str(rel_minsup)
        + "% : Mistle2: "
        + str(statistics.mean(mistle2_fold_accuracy_list))
    )
    print(
        "Relative Minsup "
        + str(rel_minsup)
        + "% : Mistle3: "
        + str(statistics.mean(mistle3_fold_accuracy_list))
    )
    print(
        "Relative Minsup "
        + str(rel_minsup)
        + "% : Mistle4: "
        + str(statistics.mean(mistle4_fold_accuracy_list))
    )

    print(
        "Relative Minsup "
        + str(rel_minsup)
        + "% : Mistle5: "
        + str(statistics.mean(mistle5_fold_accuracy_list))
    )

plt.figure()
plt.xlabel("Minimum Support Threshold")
plt.ylabel("Classification Accuracy")
plt.title("Classification on incomplete " + dataset.title() + " by varying minsup")

plt.plot(
    rel_minsups, [float(acc) for acc in krimp_accuracy_list], marker="o", label="KRIMP"
)
plt.plot(
    rel_minsups,
    mistle1_accuracy_list,
    marker="o",
    label="Mistle: classification by satisfiability",
)
plt.plot(
    rel_minsups,
    mistle2_accuracy_list,
    marker="o",
    label="Mistle: classification by %failing clauses",
)
plt.plot(
    rel_minsups,
    mistle3_accuracy_list,
    marker="o",
    label="Mistle: classification by %passing clauses",
)
plt.plot(
    rel_minsups,
    mistle4_accuracy_list,
    marker="o",
    label="Mistle: classification by shortest theory",
)
# plt.plot(
#     rel_minsups,
#     mistle5_accuracy_list,
#     marker="o",
#     label="Mistle: classification by shortest theory with errors",
# )

plt.legend()
# mplcyberpunk.add_glow_effects()
# mplcyberpunk.add_underglow()
plt.savefig(
    "Experiments/exp4_" + dataset + "_v" + str(version) + ".png", bbox_inches="tight"
)
plt.show()
plt.close()

print("KRIMP   - Accuracy\t: " + str(krimp_accuracy_list))
print("Mistle1 - Accuracy\t: " + str(mistle1_accuracy_list))
print("Mistle2 - Accuracy\t: " + str(mistle2_accuracy_list))
print("Mistle3 - Accuracy\t: " + str(mistle3_accuracy_list))
print("Mistle4 - Accuracy\t: " + str(mistle4_accuracy_list))
print("Mistle5 - Accuracy\t: " + str(mistle5_accuracy_list))
print("TIME: " + str(time() - start_time))
