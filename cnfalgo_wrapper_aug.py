import subprocess
import uuid
import os
import random
from mistle_v2 import solve, get_clauses
from collections import defaultdict


def write_train_file(positives, negatives, file="Resources/cnfalgo_train"):
    f = open(file, "w")

    for pos in positives:
        line = "0"

        l = list(pos)
        abs_l = [abs(i) for i in l]
        sorted_pos = [x for _, x in sorted(zip(abs_l, l))]

        for literal in sorted_pos:
            if literal > 0:
                line += " " + str(1000 * literal + 1)
            else:
                line += " " + str(1000 * -literal + 2)

        f.write(line + "\n")

    for neg in negatives:
        line = "1"

        l = list(neg)
        abs_l = [abs(i) for i in l]
        sorted_neg = [x for _, x in sorted(zip(abs_l, l))]

        for literal in sorted_neg:
            if literal > 0:
                line += " " + str(1000 * literal + 1)
            else:
                line += " " + str(1000 * -literal + 2)

        f.write(line + "\n")

    f.close()


def write_deletion_file(positives, negatives, file="Resources/cnfalgo_deletion"):
    f = open(file, "w")

    for i, partial_interpretation in enumerate(positives + negatives):
        if len(partial_interpretation) == 0:
            continue

        line = str(i)

        l = list(partial_interpretation)
        abs_l = [abs(i) for i in l]
        sorted_p_int = [x for _, x in sorted(zip(abs_l, l))]

        for literal in sorted_p_int:
            line += " " + str(abs(literal))

        f.write(line + "\n")

    f.close()


def write_test_file(
    test_positives,
    test_negatives,
    missing_positives,
    missing_negatives,
    file="Resources/cnfalgo_test",
):
    f = open(file, "w")
    assert len(test_positives) == len(missing_positives)
    assert len(test_negatives) == len(missing_negatives)

    for test_p, miss_p in zip(test_positives, missing_positives):
        line = "0"

        l = list(test_p) + list(miss_p)
        abs_l = [abs(i) for i in l]
        sorted_p = [x for _, x in sorted(zip(abs_l, l))]

        for literal in sorted_p:
            if literal > 0:
                line += " " + str(1000 * literal + 1)
            else:
                line += " " + str(1000 * -literal + 2)

        f.write(line + "\n")

    for test_n, miss_n in zip(test_negatives, missing_negatives):
        line = "1"

        l = list(test_n) + list(miss_n)
        abs_l = [abs(i) for i in l]
        sorted_n = [x for _, x in sorted(zip(abs_l, l))]

        for literal in sorted_n:
            if literal > 0:
                line += " " + str(1000 * literal + 1)
            else:
                line += " " + str(1000 * -literal + 2)

        f.write(line + "\n")

    f.close()


def select_prediction(candidate_predictions, theory):
    """
    Prune the candidate predictions and keep only those that fail maximum number of clauses in the theory.
    Then, as a tie-break, select the one with the minimum score.
    :param candidate_predictions:
    :param theory:
    :return:
    """
    clauses = get_clauses(theory)

    nb_clauses = defaultdict(int)

    completed_pas = []
    for pred, score in candidate_predictions:
        completed_pas.append(pred)

    for clause in clauses:
        nb_clauses[len(clause)] += 1

    # For each completion of the given example, calculate number of failing clauses of different lengths
    max_indices = []
    max_score = -1
    nb_failed_clauses_list = []
    for i, completed_pa in enumerate(completed_pas):
        nb_failing_clauses = defaultdict(int)
        for clause in clauses:
            if solve([tuple(clause)] + [(a,) for a in completed_pa]) == "UNSAT":
                nb_failing_clauses[len(clause)] += 1
        nb_failed_clauses_list.append(str(nb_failing_clauses))

        # Referred from Page 3 of 'Mining predictive kCNF expressions - Anton, Luc, Siegfried
        length_metric = 0.0
        for length, nb_clause in nb_clauses.items():
            if length in nb_failing_clauses:
                length_metric += nb_failing_clauses[length] / (length * nb_clause)

        if length_metric > max_score:
            max_indices = [i]
            max_score = length_metric
        elif length_metric == max_score:
            max_indices.append(i)

    pruned_candidates = [frozenset(completed_pas[i]) for i in max_indices]

    # Now select the pruned candidate with the minimum score.
    # pruned_candidates = [candidate_predictions[i] for i in max_indices]
    pruned_scores = [candidate_predictions[i][1] for i in max_indices]

    sorted_candidates = [x for _, x in sorted(zip(pruned_scores, pruned_candidates))]
    selected_candidate = sorted_candidates[0]

    return selected_candidate


def c1():
    return 0


def complete_cnfalgo_aug(
    train_positives,
    train_negatives,
    test_positives,
    test_negatives,
    missing_positives,
    missing_negatives,
    mistle_pos_theory=None,
    mistle_neg_theory=None,
    cnfalgo_exec_path="Resources/cnfalgo_aug",
    metric="length",
):
    uuid_string = str(uuid.uuid4())
    train_file = "Resources/cnfalgo_" + uuid_string + "_train"
    test_file = "Resources/cnfalgo_" + uuid_string + "_test"
    deletion_file = "Resources/cnfalgo_" + uuid_string + "_deletion"
    output_file = "Resources/cnfalgo_" + uuid_string + "_output"

    write_train_file(train_positives, train_negatives, train_file)
    write_test_file(
        test_positives, test_negatives, missing_positives, missing_negatives, test_file
    )
    write_deletion_file(missing_positives, missing_negatives, deletion_file)

    result = os.system(
        cnfalgo_exec_path
        + " mv "
        + train_file
        + " -t "
        + test_file
        + " -d "
        + deletion_file
        + " -o "
        + output_file
        + " -m "
        + metric
        + " >/dev/null 2>&1"  # Suppress both stdout and stderr
        # + " 2>&1"  # Suppress only stderr
    )
    print("Return Code\t: " + str(result))

    f = open(output_file, "r")
    lines = f.readlines()
    missing_rows_pos = []
    missing_rows_neg = []
    nb_clauses = int(lines[0].strip("\n"))
    nb_literals = int(lines[1].strip("\n"))

    converted_pos_predictions_list = []
    converted_neg_predictions_list = []

    for line in lines[2:]:
        line = line.strip("\n")

        predictions = []
        if ";" in line:
            for p in line.split(";")[1:]:
                predictions.append(line.split(";")[0] + p)
        else:
            predictions.append(line)

        candidate_pos_predictions = []
        candidate_neg_predictions = []
        sign = None
        for p in predictions:
            p = p.split(" ")
            fs = set()
            for l in p[2:-1]:
                l = int(l.strip(","))
                if l % 10 == 1:
                    # The atom is True if the last digit of the number is 1
                    # The atom is the first digit of the number
                    fs.add(l // 1000)
                elif l % 10 == 2:
                    # The atom is True if the last digit of the number is 2
                    # The atom is the first digit of the number
                    fs.add(-1 * (l // 1000))

            score = float(p[-1])

            if p[1] == "1,":
                # This line denotes a positive
                candidate_pos_predictions.append((frozenset(fs), score))
                missing_row = int(p[0])
                if missing_row not in missing_rows_pos:
                    missing_rows_pos.append(missing_row)
                assert sign != "-"
                sign = "+"
            elif p[1] == "2,":
                # This line denotes a negative
                candidate_neg_predictions.append((frozenset(fs), score))
                missing_row = int(p[0]) - len(test_positives)
                if missing_row not in missing_rows_neg:
                    missing_rows_neg.append(missing_row)
                assert sign != "+"
                sign = "-"

        if sign == "+":
            converted_pos_predictions_list.append(candidate_pos_predictions)
        elif sign == "-":
            converted_neg_predictions_list.append(candidate_neg_predictions)

    accuracy_aug = 0
    assert len(test_positives) == len(missing_positives)
    assert len(test_negatives) == len(missing_negatives)

    complete_train_positives = []
    complete_train_negatives = []

    for candidate_pos_predictions in converted_pos_predictions_list:
        complete_train_positives.append(candidate_pos_predictions[0][0])
    for candidate_neg_predictions in converted_neg_predictions_list:
        complete_train_negatives.append(candidate_neg_predictions[0][0])

    accuracy = 0
    for i, c_p in zip(missing_rows_pos, complete_train_positives):
        t_p = test_positives[i]
        m_p = missing_positives[i]
        if set(c_p) == set(t_p) | set(m_p):
            accuracy += 1

    for i, c_n in zip(missing_rows_neg, complete_train_negatives):
        t_n = test_negatives[i]
        m_n = missing_negatives[i]
        if set(c_n) == set(t_n) | set(m_n):
            accuracy += 1

    accuracy = accuracy / (len(test_positives) + len(test_negatives))

    complete_train_positives = []
    complete_train_negatives = []

    for candidate_pos_predictions in converted_pos_predictions_list:
        complete_train_positives.append(
            select_prediction(candidate_pos_predictions, mistle_pos_theory)
        )
    for candidate_neg_predictions in converted_neg_predictions_list:
        complete_train_negatives.append(
            select_prediction(candidate_neg_predictions, mistle_neg_theory)
        )

    for i, c_p in zip(missing_rows_pos, complete_train_positives):
        t_p = test_positives[i]
        m_p = missing_positives[i]
        if set(c_p) == set(t_p) | set(m_p):
            accuracy_aug += 1

    for i, c_n in zip(missing_rows_neg, complete_train_negatives):
        t_n = test_negatives[i]
        m_n = missing_negatives[i]
        if set(c_n) == set(t_n) | set(m_n):
            accuracy_aug += 1

    accuracy_aug = accuracy_aug / (len(test_positives) + len(test_negatives))

    print("CNF-cc Accuracy\t\t: " + str(accuracy))
    print("CNF-cc Accuracy-Aug\t: " + str(accuracy_aug))

    f.close()
    os.remove(train_file)
    os.remove(test_file)
    os.remove(deletion_file)
    os.remove(output_file)

    return nb_clauses, nb_literals, accuracy, accuracy_aug


def get_cnfalgo_theory(
    train_positives,
    train_negatives,
    cnfalgo_exec_path="Resources/cnfalgo_aug",
    pos_literal=1000,
    suppress_output=True,
):

    train_file = "Resources/cnfalgo_" + str(uuid.uuid4()) + "_train"
    output_file = "Resources/cnfalgo_" + str(uuid.uuid4()) + "_output"

    write_train_file(train_positives, train_negatives, train_file)

    # if suppress_output:
    #     stdout = open(os.devnull, "w")
    # else:
    #     stdout = subprocess.STDOUT

    print("Running CNFAlgo")
    if suppress_output:
        result = os.system(
            cnfalgo_exec_path
            + " mv "
            + train_file
            + " -o "
            + output_file
            + " >/dev/null 2>&1"
        )
    else:
        result = os.system(
            cnfalgo_exec_path + " mv " + train_file + " -o " + output_file
        )
    print("Return Code for CNFAlgo\t: " + str(result))

    # subprocess.run(
    #     [cnfalgo_exec_path, "mv", train_file, "-o", output_file],
    #     stdout=stdout,
    #     stderr=subprocess.STDOUT,
    # )

    f = open(output_file, "r")

    lines = f.readlines()

    cnfalgo_theory = []
    nb_clauses = int(lines[0].strip("\n"))
    for line in lines[1:]:
        line = line.strip("\n").split(" ")
        fs = set()

        for l in line:
            l = int(l.strip(","))
            if l == 1 or l == -2:
                # This atom corresponds to positive_class or not(negative_class)
                fs.add(pos_literal)

            elif l == 2 or l == -1:
                # This atom corresponds to negative_class or not(positive_class)
                fs.add(-pos_literal)

            elif abs(l) % 10 == 1:
                # The atom is True if the last digit of the number is 1
                # The atom is the first digit of the number
                # The truth value reverses with a negation sign
                if l > 0:
                    fs.add(abs(l) // 1000)
                else:
                    fs.add(-1 * (abs(l) // 1000))
            elif abs(l) % 10 == 2:
                # The atom is True if the last digit of the number is 2
                # The atom is the first digit of the number
                # The truth value reverses with a negation sign
                if l > 0:
                    fs.add(-1 * (abs(l) // 1000))
                else:
                    fs.add(abs(l) // 1000)

        cnfalgo_theory.append(frozenset(fs))

    assert len(cnfalgo_theory) == nb_clauses

    os.remove(train_file)
    os.remove(output_file)

    return cnfalgo_theory


if __name__ == "__main__":
    cnfalgo_exec_path = "Resources/cnfalgo_aug"
