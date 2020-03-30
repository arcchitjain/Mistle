# from mistle_class import *
# from mistle_beam import *

from mistle_v2 import *
import random
from tqdm import tqdm
import os
from pycosat import itersolve
from collections import OrderedDict
from collections import defaultdict

# def count_solutions(pa, theory):
#
#     theory_cnf = [tuple(clause) for clause in theory]
#
#     cnf = theory_cnf + [(a,) for a in pa]
#
#     return len(list(itersolve(cnf)))


def count_models(pa, theory, id=None):

    total_clauses = len(theory.clauses) + len(pa)
    total_vars = theory.new_var_counter - 1
    if id:
        filename = "./CNFs/" + id + ".cnf"
    else:
        filename = "./CNFs/0.cnf"

    f = open(filename, "w+")
    f.write(
        "c ind "
        + " ".join(
            [
                str(a)
                for a in range(
                    1, total_vars - len(theory.invented_predicate_definition)
                )
            ]
        )
        + " 0\n"
    )
    f.write("p cnf " + str(total_vars) + " " + str(total_clauses) + "\n")
    for literal in pa:
        f.write(str(literal) + " 0\n")

    for clause in theory.clauses:
        f.write(" ".join([str(a) for a in clause]) + " 0\n")

    f.close()

    if id:
        outfilename = "./CNFs/" + id + ".out"
    else:
        outfilename = "./CNFs/0.out"

    os.system(
        "cd Ganak/build/ && python ganak.py ../."
        + filename
        + " -p > ../."
        + outfilename
    )

    outf = open(outfilename, "r")
    sol_found = False
    total_models = None
    for line in outf.readlines():
        if sol_found:
            total_models = line[:-1]
            # print(total_models)
            break
        if "solutions" in line:
            sol_found = True
    outf.close()

    if total_models:
        return int(total_models)
    else:
        print("No solutions found")
        return 0


def split_data(data, num_folds=10, seed=1234):
    random.seed(seed)

    split_data = [[] for i in range(num_folds)]

    for datapoint in data:
        split_data[int(random.random() * num_folds)].append(datapoint)

    # split_data2 = [[] for i in range(num_folds)]
    #
    # for datapoint in split_data[0]:
    #     split_data2[int(random.random() * num_folds)].append(datapoint)

    return split_data


def compress(pa, definitions):
    """
    :param pa: A partial assignment as a test datapoint
    :param definitions: An ordered dictionary of definitions invented predicates
    :return: A compressed pa that is substituted with some of the invented predicates
    """

    # substituted_definitions = {}
    # for new_predicate, definition in definitions.items():
    #     substituted_definition = set()
    #     for literal in definition:
    #         if literal not in definitions:
    #             substituted_definition.add(literal)
    #         else:
    #             # TODO
    #             pass
    #
    #     substituted_definitions[new_predicate] = definition

    compressed_pa = set(pa)
    # definitions = copy(definitions)

    while definitions:
        new_pred, definition = definitions.popitem(last=False)
        if definition.issubset(compressed_pa):
            compressed_pa = compressed_pa - definition
            compressed_pa.add(new_pred)

    return compressed_pa


def test_theory(theory, positives, negatives):
    accuracy = 0
    total_datapoints = len(positives) + len(negatives)

    pbar = tqdm(total=total_datapoints)
    pbar.set_description("Testing theory")

    if theory is None:
        clauses = None
    else:
        clauses = theory.clauses

    TP_s = 0
    TN_s = 0
    FP_s = 0
    FN_s = 0

    for pa in positives:
        if check_pa_satisfiability(pa, clauses):
            # Correctly classified as a positive
            accuracy += 1
            TP_s += 1
        else:
            # Wrongly classified as a negative
            FN_s += 1
        pbar.update(1)

    for pa in negatives:
        if not check_pa_satisfiability(pa, clauses):
            # Correctly classified as a negative
            accuracy += 1
            TN_s += 1
        else:
            # Wrongly classified as a positive
            FP_s += 1
        pbar.update(1)

    pbar.close()

    return (
        float(accuracy) / total_datapoints,
        1.0,
        {"TP_s": TP_s, "TN_s": TN_s, "FP_s": FP_s, "FN_s": FN_s},
    )


def test_both_theories_by_compression(
    pos_theory, neg_theory, test_positives, test_negatives
):
    accuracy = 0
    total_classifications = 0

    pbar = tqdm(total=len(test_positives) + len(test_negatives))
    pbar.set_description("Testing both theories using SAT + Counting")

    TP_c = 0
    TN_c = 0
    FP_c = 0
    FN_c = 0

    TP_s = 0
    TN_s = 0
    FP_s = 0
    FN_s = 0

    if pos_theory is None:
        p_clauses = None
    else:
        p_clauses = pos_theory.clauses

    if neg_theory is None:
        n_clauses = None
    else:
        n_clauses = neg_theory.clauses

    # Remove negative signs from the literals in the definitions
    abs_pos_def = OrderedDict()
    for var, definition in pos_theory.invented_predicate_definition.items():
        abs_def = set()
        for literal in definition:
            abs_def.add(-1 * literal)
        abs_pos_def[var] = abs_def

    abs_neg_def = OrderedDict()
    for var, definition in neg_theory.invented_predicate_definition.items():
        abs_def = set()
        for literal in definition:
            abs_def.add(-1 * literal)
        abs_neg_def[var] = abs_def

    for pa in test_positives:
        p = check_pa_satisfiability(pa, p_clauses)
        n = check_pa_satisfiability(pa, n_clauses)

        if not p and n:
            # Correctly classified as a positive
            accuracy += 1
            TP_s += 1
            total_classifications += 1
        elif p and not n:
            # Wrongly classified as a negative
            FN_s += 1
            total_classifications += 1
        else:
            pos_length = len(compress(pa, copy(abs_pos_def)))
            neg_length = len(compress(pa, copy(abs_neg_def)))

            if pos_length < neg_length:
                # Correctly classified as a positive
                accuracy += 1
                TP_c += 1
                total_classifications += 1
            elif neg_length < pos_length:
                # Wrongly classified as a negative
                FN_c += 1
                total_classifications += 1

        pbar.update(1)

    for pa in test_negatives:
        p = check_pa_satisfiability(pa, p_clauses)
        n = check_pa_satisfiability(pa, n_clauses)

        if not p and n:
            # Wrongly classified as a positive
            FP_s += 1
            total_classifications += 1
        elif p and not n:
            # Correctly classified as a negative
            accuracy += 1
            TN_s += 1
            total_classifications += 1
        else:
            pos_length = len(compress(pa, copy(abs_pos_def)))
            neg_length = len(compress(pa, copy(abs_neg_def)))

            if pos_length < neg_length:
                # Wrongly classified as a positive
                FP_c += 1
                total_classifications += 1
            elif neg_length < pos_length:
                # Correctly classified as a negative
                accuracy += 1
                TN_c += 1
                total_classifications += 1

        pbar.update(1)

    pbar.close()

    print(
        "Confusion Matrix \t: TP_s = "
        + str(TP_s)
        + "; TN_s = "
        + str(TN_s)
        + "; FP_s = "
        + str(FP_s)
        + "; FN_s = "
        + str(FN_s)
        + "; TP_c = "
        + str(TP_c)
        + "; TN_c = "
        + str(TN_c)
        + "; FP_c = "
        + str(FP_c)
        + "; FN_c = "
        + str(FN_c)
    )

    if total_classifications == 0:
        return None, 0
    else:
        return (
            float(accuracy) / total_classifications,
            float(total_classifications) / (len(test_positives) + len(test_negatives)),
            {
                "TP_s": TP_s,
                "TN_s": TN_s,
                "FP_s": FP_s,
                "FN_s": FN_s,
                "TP_c": TP_c,
                "TN_c": TN_c,
                "FP_c": FP_c,
                "FN_c": FN_c,
            },
        )


def test_both_theories_by_counting(
    pos_theory, neg_theory, test_positives, test_negatives
):
    accuracy = 0
    total_classifications = 0

    pbar = tqdm(total=len(test_positives) + len(test_negatives))
    pbar.set_description("Testing both theories using SAT + Compression")

    TP_c = 0
    TN_c = 0
    FP_c = 0
    FN_c = 0

    TP_s = 0
    TN_s = 0
    FP_s = 0
    FN_s = 0

    if pos_theory is None:
        p_clauses = None
    else:
        p_clauses = pos_theory.clauses

    if neg_theory is None:
        n_clauses = None
    else:
        n_clauses = neg_theory.clauses

    for i, pa in enumerate(test_positives):
        p = check_pa_satisfiability(pa, p_clauses)
        n = check_pa_satisfiability(pa, n_clauses)

        if not p and n:
            # Correctly classified as a positive
            accuracy += 1
            total_classifications += 1
            TP_s += 1
        elif p and not n:
            # Wrongly classified as a negative
            total_classifications += 1
            FN_s += 1
        elif p and n:
            pos_models = count_models(pa, pos_theory, str(i) + "+")
            neg_models = count_models(pa, neg_theory, str(i) + "-")

            if pos_models < neg_models:
                # Correctly classified as a positive
                accuracy += 1
                total_classifications += 1
                TP_c += 1
            elif neg_models < pos_models:
                # Wrongly classified as a negative
                total_classifications += 1
                FN_c += 1
        pbar.update(1)

    i = len(test_positives)
    for j, pa in enumerate(test_negatives):
        p = check_pa_satisfiability(pa, p_clauses)
        n = check_pa_satisfiability(pa, n_clauses)

        if not p and n:
            # Wrongly classified as a positive
            total_classifications += 1
            FP_s += 1
        elif p and not n:
            # Correctly classified as a negative
            accuracy += 1
            total_classifications += 1
            TN_s += 1
        elif p and n:
            pos_models = count_models(pa, pos_theory, str(j + i) + "+")
            neg_models = count_models(pa, neg_theory, str(j + i) + "-")

            if pos_models < neg_models:
                # Wrongly classified as a positive
                total_classifications += 1
                FP_c += 1
            elif neg_models < pos_models:
                # Correctly classified as a negative
                accuracy += 1
                total_classifications += 1
                TN_c += 1

        pbar.update(1)

    pbar.close()

    print(
        "Confusion Matrix \t: TP_s = "
        + str(TP_s)
        + "; TN_s = "
        + str(TN_s)
        + "; FP_s = "
        + str(FP_s)
        + "; FN_s = "
        + str(FN_s)
        + "; TP_c = "
        + str(TP_c)
        + "; TN_c = "
        + str(TN_c)
        + "; FP_c = "
        + str(FP_c)
        + "; FN_c = "
        + str(FN_c)
    )

    if total_classifications == 0:
        return None, 0
    else:
        return (
            float(accuracy) / total_classifications,
            float(total_classifications) / (len(test_positives) + len(test_negatives)),
            {
                "TP_s": TP_s,
                "TN_s": TN_s,
                "FP_s": FP_s,
                "FN_s": FN_s,
                "TP_c": TP_c,
                "TN_c": TN_c,
                "FP_c": FP_c,
                "FN_c": FN_c,
            },
        )


def test_both_theories_by_satisfiability(
    pos_theory, neg_theory, test_positives, test_negatives, default_prediction=None
):
    accuracy = 0

    pbar = tqdm(total=len(test_positives) + len(test_negatives))
    if default_prediction is not None:
        pbar.set_description("Testing both theories using SAT + default prediction")
    else:
        pbar.set_description("Testing both theories using SAT")

    total_classifications = 0

    TP_s = 0
    TN_s = 0
    FP_s = 0
    FN_s = 0

    TP_f = 0
    TN_f = 0
    FP_f = 0
    FN_f = 0

    if pos_theory is None:
        p_clauses = None
    else:
        p_clauses = pos_theory.clauses

    if neg_theory is None:
        n_clauses = None
    else:
        n_clauses = neg_theory.clauses

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
            total_classifications += 1
        elif (
            (p is True and n is False)
            or (p is None and n is False)
            or (p is True and n is None)
        ):
            # Wrongly classified as a negative
            FN_s += 1
            total_classifications += 1
        else:
            if default_prediction == "+":
                accuracy += 1
                TP_f += 1
                total_classifications += 1
            elif default_prediction == "-":
                FN_f += 1
                total_classifications += 1
        pbar.update(1)

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
            total_classifications += 1
        elif (
            (p is True and n is False)
            or (p is None and n is False)
            or (p is True and n is None)
        ):
            # Correctly classified as a negative
            accuracy += 1
            TN_s += 1
            total_classifications += 1
        else:
            if default_prediction == "+":
                FP_f += 1
                total_classifications += 1
            elif default_prediction == "-":
                accuracy += 1
                TN_f += 1
                total_classifications += 1
        pbar.update(1)

    pbar.close()
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
        float(accuracy) / total_classifications,
        float(total_classifications) / (len(test_positives) + len(test_negatives)),
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


def cross_validate(
    positives,
    negatives,
    num_folds=10,
    output_file=None,
    test_both=False,
    minsup=10,
    dl_measure="se",
):
    start_time = time()
    avg_accuracy = 0.0
    if test_both:
        avg_compression = [0.0, 0.0]
    else:
        avg_compression = 0

    avg_coverage = 0.0
    seed = 1234
    ignore_folds = 0
    total_datapoints = len(positives) + len(negatives)

    split_positives = split_data(positives, num_folds, seed=seed)
    split_negatives = split_data(negatives, num_folds, seed=seed)

    confusions = defaultdict(int)

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
            pbar = tqdm(total=2 * total_datapoints)
            pbar.set_description("Outputting data")

            if not os.path.exists("./Output/" + output_file):
                os.makedirs("./Output/" + output_file)

            with open(
                "./Output/"
                + output_file
                + "/"
                + output_file
                + "_"
                + str(fold)
                + "_train_pos.dat",
                "w+",
            ) as f:
                for pa in train_positives:
                    l = list(pa)
                    abs_l = [abs(i) for i in l]
                    f.write(" ".join([str(x) for _, x in sorted(zip(abs_l, l))]) + "\n")
                    pbar.update(1)

            with open(
                "./Output/"
                + output_file
                + "/"
                + output_file
                + "_"
                + str(fold)
                + "_train_neg.dat",
                "w+",
            ) as f:
                for pa in train_negatives:
                    l = list(pa)
                    abs_l = [abs(i) for i in l]
                    f.write(" ".join([str(x) for _, x in sorted(zip(abs_l, l))]) + "\n")
                    pbar.update(1)

            with open(
                "./Output/"
                + output_file
                + "/"
                + output_file
                + "_"
                + str(fold)
                + "_test_pos.dat",
                "w+",
            ) as f:
                for pa in test_positives:
                    l = list(pa)
                    abs_l = [abs(i) for i in l]
                    f.write(" ".join([str(x) for _, x in sorted(zip(abs_l, l))]) + "\n")
                    pbar.update(1)

            with open(
                "./Output/"
                + output_file
                + "/"
                + output_file
                + "_"
                + str(fold)
                + "_test_neg.dat",
                "w+",
            ) as f:
                for pa in test_negatives:
                    l = list(pa)
                    abs_l = [abs(i) for i in l]
                    f.write(" ".join([str(x) for _, x in sorted(zip(abs_l, l))]) + "\n")
                    pbar.update(1)

            pbar.close()

        # Compressed Theory
        if test_both:
            pos_mistle = Mistle(train_negatives, train_positives)
            pos_theory, pos_compression = pos_mistle.learn(
                minsup=minsup, dl_measure=dl_measure
            )

            neg_mistle = Mistle(train_positives, train_negatives)
            neg_theory, neg_compression = neg_mistle.learn(
                minsup=minsup, dl_measure=dl_measure
            )

            # fold_accuracy, coverage, fold_confusions = test_both_theories_by_compression(
            #     pos_theory, neg_theory, test_positives, test_negatives
            # )

            # fold_accuracy, coverage, fold_confusions = test_both_theories_by_counting(
            #     pos_theory, neg_theory, test_positives, test_negatives
            # )

            if len(train_positives) > len(train_negatives):
                default_prediction = "+"
            else:
                default_prediction = "-"

            (
                fold_accuracy,
                coverage,
                fold_confusions,
            ) = test_both_theories_by_satisfiability(
                pos_theory,
                neg_theory,
                test_positives,
                test_negatives,
                default_prediction,
            )

            print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy))
            print(
                "Coverage of fold "
                + str(fold)
                + "\t: "
                + str(round(coverage * 100, 2))
                + "\n"
            )

            avg_compression[0] += pos_compression
            avg_compression[1] += neg_compression

        else:
            mistle = Mistle(train_positives, train_negatives)
            theory, compression = mistle.learn(minsup=minsup, dl_measure=dl_measure)

            fold_accuracy, coverage, fold_confusions = test_theory(
                theory, test_positives, test_negatives
            )
            print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy) + "\n")

            avg_compression += compression

        for k, v in fold_confusions.items():
            confusions[k] += v

        avg_coverage += coverage

        if fold_accuracy is not None:
            avg_accuracy += fold_accuracy
        else:
            ignore_folds += 1

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

    avg_accuracy = avg_accuracy / (num_folds - ignore_folds)
    if test_both:
        avg_compression[0] = avg_compression[0] / num_folds
        avg_compression[1] = avg_compression[1] / num_folds
        avg_coverage = avg_coverage / num_folds
        print("Average Coverage \t\t\t\t\t: " + str(round(avg_coverage * 100, 2)) + "%")
    else:
        avg_compression = avg_compression / num_folds

    print(
        "Average Classification Accuracy \t: " + str(round(avg_accuracy * 100, 2)) + "%"
    )
    print("Average Compression \t\t\t\t: " + str(avg_compression) + "%")
    print("Total Time \t\t\t\t\t\t\t: " + str(time() - start_time) + " seconds")
    return avg_accuracy


if __name__ == "__main__":
    filename = sys.argv[1]
    size = int(sys.argv[2])
    minsup = int(sys.argv[3])
    if len(sys.argv) > 4:
        output_file = sys.argv[4]
    else:
        output_file = None

    positives, negatives = load_dataset(
        filename,
        2 * size,
        list(range(1, size + 1)),
        [str(size + 1), str(size + 2)],
        negation=False,
        load_top_k=None,
        switch_signs=False,
        num_vars=100,
        load_tqdm=True,
        raw_data=True,
    )
    cross_validate(
        positives,
        negatives,
        num_folds=10,
        output_file="wff_3_100_150_100_100_20_data",
        test_both=True,
        minsup=minsup,
        dl_measure="ce",
    )
