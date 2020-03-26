# from mistle_class import *
# from mistle_beam import *

from mistle_v2 import *
import random
from tqdm import tqdm
import os
from pycosat import itersolve
from collections import OrderedDict

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

    for pa in positives:
        if check_pa_satisfiability(pa, theory.clauses):
            accuracy += 1
        pbar.update(1)

    for pa in negatives:
        if not check_pa_satisfiability(pa, theory.clauses):
            accuracy += 1
        pbar.update(1)

    pbar.close()

    return float(accuracy) / total_datapoints


def test_both_theories_by_compression(
    pos_theory, neg_theory, test_positives, test_negatives
):
    accuracy = 0
    total_classifications = 0

    pbar = tqdm(total=len(test_positives) + len(test_negatives))
    pbar.set_description("Testing both theories")
    ff = 0
    ft = 0
    tf = 0
    tt = 0

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
        p = check_pa_satisfiability(pa, pos_theory.clauses)
        n = check_pa_satisfiability(pa, neg_theory.clauses)

        if not p and n:
            # Correctly classified as a positive
            accuracy += 1
            total_classifications += 1
            ft += 1
        elif p and not n:
            # Wrongly classified as a negative
            total_classifications += 1
            tf += 1
        else:
            # elif p and n:
            #     tt += 1
            # elif not p and not n:
            #     ff += 1

            pos_length = len(compress(pa, copy(abs_pos_def)))
            neg_length = len(compress(pa, copy(abs_neg_def)))

            # if not pos_definitions or not pos_definitions:
            #     print("Afsoos")
            # elif pos_length != neg_length:
            #     print("Yo\t: " + str(pos_length) + " != " + str(neg_length))

            if pos_length < neg_length:
                # Correctly classified as a positive
                accuracy += 1
                total_classifications += 1
                ft += 1
            elif neg_length < pos_length:
                # Wrongly classified as a negative
                total_classifications += 1
                tf += 1
            elif p and n:
                tt += 1
            elif not p and not n:
                ff += 1

        pbar.update(1)

    for pa in test_negatives:
        p = check_pa_satisfiability(pa, pos_theory.clauses)
        n = check_pa_satisfiability(pa, neg_theory.clauses)

        if not p and n:
            # Wrongly classified as a positive
            total_classifications += 1
            ft += 1
        elif p and not n:
            # Correctly classified as a negative
            accuracy += 1
            total_classifications += 1
            tf += 1
        else:
            # elif p and n:
            #     tt += 1
            # elif not p and not n:
            #     ff += 1
            pos_length = len(compress(pa, copy(abs_pos_def)))
            neg_length = len(compress(pa, copy(abs_neg_def)))

            # if not pos_definitions or not pos_definitions:
            #     print("Afsoos")
            # elif pos_length != neg_length:
            #     print("Yo\t: " + str(pos_length) + " != " + str(neg_length))

            if pos_length < neg_length:
                # Wrongly classified as a positive
                total_classifications += 1
                ft += 1
            elif neg_length < pos_length:
                # Correctly classified as a negative
                accuracy += 1
                total_classifications += 1
                tf += 1
            elif p and n:
                tt += 1
            elif not p and not n:
                ff += 1

        pbar.update(1)

    pbar.close()
    print(
        "Classification Matrix \t: ff = "
        + str(ff)
        + "; ft = "
        + str(ft)
        + "; tf = "
        + str(tf)
        + "; tt = "
        + str(tt)
    )

    if total_classifications == 0:
        return None, 0
    else:
        return (
            float(accuracy) / total_classifications,
            float(total_classifications) / (len(test_positives) + len(test_negatives)),
        )


def test_both_theories_by_counting(
    pos_theory, neg_theory, test_positives, test_negatives
):
    accuracy = 0
    total_classifications = 0

    pbar = tqdm(total=len(test_positives) + len(test_negatives))
    pbar.set_description("Testing both theories")
    ff = 0
    ft = 0
    tf = 0
    tt = 0
    ft_c = 0
    tf_c = 0

    TP_c = 0
    TN_c = 0
    FP_c = 0
    FN_c = 0

    TP_s = 0
    TN_s = 0
    FP_s = 0
    FN_s = 0

    for i, pa in enumerate(test_positives):
        p = check_pa_satisfiability(pa, pos_theory.clauses)
        n = check_pa_satisfiability(pa, neg_theory.clauses)

        if not p and n:
            # Correctly classified as a positive
            accuracy += 1
            total_classifications += 1
            ft += 1
            TP_s += 1
        elif p and not n:
            # Wrongly classified as a negative
            total_classifications += 1
            tf += 1
            FN_s += 1
        elif p and n:
            # pos_models = count_models(pa, pos_theory, str(i) + "+")
            # neg_models = count_models(pa, neg_theory, str(i) + "-")
            #
            # if pos_models < neg_models:
            #     # Correctly classified as a positive
            #     accuracy += 1
            #     total_classifications += 1
            #     ft_c += 1
            #     TP_c += 1
            # elif neg_models < pos_models:
            #     # Wrongly classified as a negative
            #     total_classifications += 1
            #     tf_c += 1
            #     FN_c += 1
            # else:
            tt += 1
        elif not p and not n:
            ff += 1
        pbar.update(1)

    i = len(test_positives)
    for j, pa in enumerate(test_negatives):
        p = check_pa_satisfiability(pa, pos_theory.clauses)
        n = check_pa_satisfiability(pa, neg_theory.clauses)

        if not p and n:
            # Wrongly classified as a positive
            total_classifications += 1
            ft += 1
            FP_s += 1
        elif p and not n:
            # Correctly classified as a negative
            accuracy += 1
            total_classifications += 1
            tf += 1
            TN_s += 1
        elif p and n:
            # pos_models = count_models(pa, pos_theory, str(j + i) + "+")
            # neg_models = count_models(pa, neg_theory, str(j + i) + "-")
            #
            # if pos_models < neg_models:
            #     # Wrongly classified as a positive
            #     total_classifications += 1
            #     ft_c += 1
            #     FP_c += 1
            # elif neg_models < pos_models:
            #     # Correctly classified as a negative
            #     accuracy += 1
            #     total_classifications += 1
            #     tf_c += 1
            #     TN_c += 1
            # else:
            tt += 1
        elif not p and not n:
            ff += 1
        pbar.update(1)

    pbar.close()
    print(
        "Classification Matrix \t: ff = "
        + str(ff)
        + "; ft = "
        + str(ft)
        + "; tf = "
        + str(tf)
        + "; tt = "
        + str(tt)
        + "; ft_c = "
        + str(ft_c)
        + "; tf_c = "
        + str(tf_c)
    )

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
        )


def cross_validate(
    positives, negatives, num_folds=10, output_file=None, test_both=False, minsup=10,
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
        if test_both:
            pos_mistle = Mistle(train_negatives, train_positives)
            pos_theory, pos_compression = pos_mistle.learn(minsup=minsup)

            neg_mistle = Mistle(train_positives, train_negatives)
            neg_theory, neg_compression = neg_mistle.learn(minsup=minsup)

            # fold_accuracy, coverage = test_both_theories_by_compression(
            #     pos_theory, neg_theory, test_positives, test_negatives
            # )

            fold_accuracy, coverage = test_both_theories_by_counting(
                pos_theory, neg_theory, test_positives, test_negatives
            )

            print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy))
            print(
                "Coverage of fold " + str(fold) + "\t: " + str(round(coverage * 100, 2))
            )

            avg_compression[0] += pos_compression
            avg_compression[1] += neg_compression

            avg_coverage += coverage
        else:
            mistle = Mistle(train_positives, train_negatives)
            theory, compression = mistle.learn(minsup=minsup)

            fold_accuracy = test_theory(theory, test_positives, test_negatives)
            print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy))

            avg_compression += compression

        # theory = mistle.convert_to_theory(train_negatives)
        # compression = 0
        # pos_theory = Mistle(train_positives, train_negatives).learn()
        # neg_theory = Mistle(train_negatives, train_positives).learn()

        # Uncompressed Theory
        # mistle = Mistle(train_positives, train_negatives)
        # theory = mistle.convert_to_theory(train_negatives)

        if fold_accuracy is not None:
            avg_accuracy += fold_accuracy
        else:
            ignore_folds += 1

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


# positives, negatives = load_breast()
# positives, negatives = load_pima()
# positives, negatives = load_ionosphere()
# positives, negatives = load_tictactoe()
# cross_validate(positives, negatives, 10, lossless=False, test_both=True)
# cross_validate(negatives, positives, 10, lossless=False, test_both=False)

# positives, negatives = load_chess(switch_signs=True)
# positives, negatives = load_mushroom()
# cross_validate(positives, negatives, 10, "./Output/adult", lossless=False)
# cross_validate(positives, negatives, 10, "./Output/mushroom", lossless=False)
# cross_validate(positives, negatives, 10, lossless=False, test_both=False)

positives, negatives = load_tictactoe()
cross_validate(positives, negatives, num_folds=10, test_both=True, minsup=2)
# positives, negatives = load_ionosphere()
# cross_validate(positives, negatives, num_folds=10, test_both=True, minsup=90)
# positives, negatives = load_breast()
# cross_validate(positives, negatives, num_folds=10, lossless=False, test_both=True)
# positives, negatives = load_pima()
# cross_validate(positives, negatives, num_folds=10, lossless=False, test_both=True)
# positives, negatives = load_chess()
# cross_validate(negatives, positives, num_folds=10, lossless=False, test_both=False)
# cross_validate(positives, negatives, num_folds=10, lossless=False, test_both=True)
# positives, negatives = load_adult()
# cross_validate(positives, negatives, num_folds=10, lossless=False, test_both=True)
# positives, negatives = load_mushroom()
# cross_validate(positives, negatives, 10, lossless=False, test_both=True)
