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


def test_both_theories(pos_theory, neg_theory, test_positives, test_negatives):
    accuracy = 0
    total_classifications = 0

    pbar = tqdm(total=len(test_positives) + len(test_negatives))
    pbar.set_description("Testing both theories")
    ff = 0
    ft = 0
    tf = 0
    tt = 0

    for pa in test_positives:
        p = check_pa_satisfiability(pa, pos_theory)
        n = check_pa_satisfiability(pa, neg_theory)

        if not p and n:
            # Correctly classified as a positive
            accuracy += 1
            total_classifications += 1
            ft += 1
        elif p and not n:
            # Wrongly classified as a negative
            total_classifications += 1
            tf += 1
        elif p and n:
            tt += 1
        elif not p and not n:
            ff += 1

        pbar.update(1)

    for pa in test_negatives:
        p = check_pa_satisfiability(pa, pos_theory)
        n = check_pa_satisfiability(pa, neg_theory)

        if not p and n:
            # Wrongly classified as a positive
            total_classifications += 1
            ft += 1
        elif p and not n:
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


# def test_both_theories_intersolve(pos_theory, neg_theory, positives, negatives):
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
    positives,
    negatives,
    num_folds=10,
    output_file=None,
    lossless=False,
    test_both=False,
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
            pos_theory, pos_compression = pos_mistle.learn(lossless=lossless)

            neg_mistle = Mistle(train_positives, train_negatives)
            neg_theory, neg_compression = neg_mistle.learn(lossless=lossless)

            fold_accuracy, coverage = test_both_theories(
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
            theory, compression = mistle.learn(lossless=lossless)

            fold_accuracy = test_theory(theory, test_positives, test_negatives)
            print("Accuracy of fold " + str(fold) + "\t: " + str(fold_accuracy))

            avg_compression += compression

        # theory = mistle.convert_to_theory(train_negatives)
        # compression = 0
        # pos_theory = Mistle(train_positives, train_negatives).learn(lossless=lossless)
        # neg_theory = Mistle(train_negatives, train_positives).learn(lossless=lossless)

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


positives, negatives = load_breast()
# cross_validate(positives, negatives, 10, lossless=False, test_both=False)
cross_validate(negatives, positives, 10, lossless=False, test_both=False)
positives, negatives = load_pima()
# cross_validate(positives, negatives, 10, lossless=False, test_both=False)
cross_validate(negatives, positives, 10, lossless=False, test_both=False)
positives, negatives = load_ionosphere()
# cross_validate(positives, negatives, 10, lossless=False, test_both=False)
cross_validate(negatives, positives, 10, lossless=False, test_both=False)
positives, negatives = load_tictactoe()
# cross_validate(positives, negatives, 10, lossless=False, test_both=False)
cross_validate(negatives, positives, 10, lossless=False, test_both=False)

# positives, negatives = load_chess(switch_signs=True)
# positives, negatives = load_mushroom()
# cross_validate(positives, negatives, 10, "./Output/adult", lossless=False)
# cross_validate(positives, negatives, 10, "./Output/mushroom", lossless=False)
# cross_validate(positives, negatives, 10, lossless=False, test_both=False)
