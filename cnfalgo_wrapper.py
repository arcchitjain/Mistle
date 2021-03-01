import subprocess
import uuid
import os
import random


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


def write_train_file_cc(positives, negatives, file="Resources/cnfalgo_train"):
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


def write_deletion_file_cc(positives, negatives, file="Resources/cnfalgo_deletion"):
    f = open(file, "w")

    for i, partial_interpretation in enumerate(positives + negatives):
        f.write(str(i) + " 1\n")

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


def complete_cnfalgo(
    train_positives,
    train_negatives,
    test_positives,
    test_negatives,
    missing_positives,
    missing_negatives,
    cnfalgo_exec_path="Resources/cnfalgo",
    metric="length",
    suppress_output=False,
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

    # if suppress_output:
    #     stdout = open(os.devnull, "w")
    # else:
    #     stdout = subprocess.STDOUT

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
    # subprocess.run(
    #     [
    #         cnfalgo_exec_path,
    #         "mv",
    #         train_file,
    #         "-t",
    #         test_file,
    #         "-d",
    #         deletion_file,
    #         "-o",
    #         output_file,
    #         "-m",
    #         metric,
    #     ],
    #     stdout=stdout,
    #     stderr=subprocess.STDOUT,
    # )

    f = open(output_file, "r")
    nb_tied_examples = 0
    nb_ties = 0
    lines = f.readlines()
    missing_rows_pos = []
    missing_rows_neg = []
    complete_train_positives = []
    complete_train_negatives = []
    nb_clauses = int(lines[0].strip("\n"))
    nb_literals = int(lines[1].strip("\n"))

    for line in lines[2:]:
        line = line.strip("\n")
        if ";" in line:
            nb_tied_examples += 1
            first_space_index = line.find(" ")
            candidate_predictions = line[first_space_index + 1 :].split(";")
            nb_ties += len(candidate_predictions)
            chosen_prediction = line[: first_space_index + 1] + random.choice(
                candidate_predictions
            )
            line = chosen_prediction
            print(line)

        line = line.split(" ")
        fs = set()
        for l in line[2:]:
            l = int(l.strip(","))
            if l % 10 == 1:
                # The atom is True if the last digit of the number is 1
                # The atom is the first digit of the number
                fs.add(l // 1000)
            elif l % 10 == 2:
                # The atom is True if the last digit of the number is 2
                # The atom is the first digit of the number
                fs.add(-1 * (l // 1000))

        candidate_predictions = []
        candidate_predictions.append(line)

        # if abs(l) % 10 == 1:
        #     # The atom is True if the last digit of the number is 1
        #     # The atom is the first digit of the number
        #     # The truth value reverses with a negation sign
        #     if l > 0:
        #         fs.add(abs(l) // 1000)
        #     else:
        #         fs.add(-1 * (abs(l) // 1000))
        # elif abs(l) % 10 == 2:
        #     # The atom is True if the last digit of the number is 2
        #     # The atom is the first digit of the number
        #     # The truth value reverses with a negation sign
        #     if l > 0:
        #         fs.add(-1 * (abs(l) // 1000))
        #     else:
        #         fs.add(abs(l) // 1000)

        if line[1] == "1,":
            # This line denotes a positive
            # print("Complete Train Positive\t: " + str(fs))
            complete_train_positives.append(frozenset(fs))
            missing_rows_pos.append(int(line[0]))

        elif line[1] == "2,":
            # This line denotes a negative
            # print("Complete Train Negative\t: " + str(fs))
            complete_train_negatives.append(frozenset(fs))
            missing_rows_neg.append(int(line[0]) - len(test_positives))

    accuracy = 0
    assert len(test_positives) == len(missing_positives)
    assert len(test_negatives) == len(missing_negatives)

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

    # for c_p, t_p, m_p in zip(
    #     complete_train_positives, test_positives, missing_positives
    # ):
    #     if set(c_p) == set(t_p) | set(m_p):
    #         accuracy += 1

    # for c_n, t_n, m_n in zip(
    #     complete_train_negatives, test_negatives, missing_negatives
    # ):
    #     if set(c_n) == set(t_n) | set(m_n):
    #         accuracy += 1

    accuracy = accuracy / (len(test_positives) + len(test_negatives))

    print("CNF-cc Accuracy\t\t: " + str(accuracy))

    os.remove(train_file)
    os.remove(test_file)
    os.remove(deletion_file)
    os.remove(output_file)

    return nb_clauses, nb_literals, accuracy, (nb_tied_examples, nb_ties)


def classify_cnfalgo(
    train_positives,
    train_negatives,
    test_positives,
    test_negatives,
    missing_positives,
    missing_negatives,
    cnfalgo_exec_path="Resources/cnfalgo",
    metric="length",
    suppress_output=False,
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
    write_deletion_file_cc(missing_positives, missing_negatives, deletion_file)

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
    nb_tied_examples = 0
    nb_ties = 0
    lines = f.readlines()
    missing_rows_pos = []
    missing_rows_neg = []
    complete_train_positives = []
    complete_train_negatives = []
    nb_clauses = int(lines[0].strip("\n"))
    nb_literals = int(lines[1].strip("\n"))

    accuracy = 0
    for line in lines[2:]:
        line = line.strip("\n")
        if ";" in line:
            nb_tied_examples += 1
            first_space_index = line.find(" ")
            candidate_predictions = line[first_space_index + 1 :].split(";")
            nb_ties += len(candidate_predictions)
            chosen_prediction = line[: first_space_index + 1] + random.choice(
                candidate_predictions
            )
            line = chosen_prediction
            print(line)

        line = line.split(" ")
        # fs = set()
        # for l in line[2:]:
        #     l = int(l.strip(","))
        #     if l % 10 == 1:
        #         # The atom is True if the last digit of the number is 1
        #         # The atom is the first digit of the number
        #         fs.add(l // 1000)
        #     elif l % 10 == 2:
        #         # The atom is True if the last digit of the number is 2
        #         # The atom is the first digit of the number
        #         fs.add(-1 * (l // 1000))

        candidate_predictions = []
        candidate_predictions.append(line)

        # if abs(l) % 10 == 1:
        #     # The atom is True if the last digit of the number is 1
        #     # The atom is the first digit of the number
        #     # The truth value reverses with a negation sign
        #     if l > 0:
        #         fs.add(abs(l) // 1000)
        #     else:
        #         fs.add(-1 * (abs(l) // 1000))
        # elif abs(l) % 10 == 2:
        #     # The atom is True if the last digit of the number is 2
        #     # The atom is the first digit of the number
        #     # The truth value reverses with a negation sign
        #     if l > 0:
        #         fs.add(-1 * (abs(l) // 1000))
        #     else:
        #         fs.add(abs(l) // 1000)

        if line[1] == "1,":
            # This line denotes a positive
            if int(line[0]) < len(test_positives):
                accuracy += 1

        elif line[1] == "2,":
            # This line denotes a negative
            if int(line[0]) >= len(test_positives):
                accuracy += 1

    accuracy = accuracy / (len(test_positives) + len(test_negatives))

    print("CNF-cc Accuracy\t\t: " + str(accuracy))

    os.remove(train_file)
    os.remove(test_file)
    os.remove(deletion_file)
    os.remove(output_file)

    return nb_clauses, nb_literals, accuracy, (nb_tied_examples, nb_ties)


def get_cnfalgo_theory(
    train_positives,
    train_negatives,
    cnfalgo_exec_path="Resources/cnfalgo",
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
    cnfalgo_exec_path = "Resources/cnfalgo"

    train_positives = []
    train_positives.append(frozenset([1, 2]))
    train_positives.append(frozenset([2, 3]))
    train_positives.append(frozenset([2, 4]))
    train_negatives = []
    train_negatives.append(frozenset([-1, -2, -3, -4, -5]))
    train_negatives.append(frozenset([-1, -2, -3, -6, -7]))
    train_negatives.append(frozenset([-1, -3, -4]))
    train_negatives.append(frozenset([-2, -5, -6]))
    train_negatives.append(frozenset([-2, -3, -4]))
    train_negatives.append(frozenset([-2, -3, 4]))
    train_negatives.append(frozenset([-1, -3, -4]))
    train_negatives.append(frozenset([-2, 3]))

    test_positives = []
    test_positives.append(frozenset([1]))
    test_positives.append(frozenset([2]))
    test_positives.append(frozenset([2]))
    test_negatives = []
    test_negatives.append(frozenset([-1, -3, -4, -5]))
    test_negatives.append(frozenset([-1, -3, -6, -7]))
    test_negatives.append(frozenset([-1, -4]))
    test_negatives.append(frozenset([-2, -6]))
    test_negatives.append(frozenset([-2, -4]))
    test_negatives.append(frozenset([-2, 4]))
    test_negatives.append(frozenset([-1, -4]))
    test_negatives.append(frozenset([-2]))

    missing_positives = []
    missing_positives.append(frozenset([2]))
    missing_positives.append(frozenset([3]))
    missing_positives.append(frozenset([4]))
    missing_negatives = []
    missing_negatives.append(frozenset([-2]))
    missing_negatives.append(frozenset([-2]))
    missing_negatives.append(frozenset([-3]))
    missing_negatives.append(frozenset([-5]))
    missing_negatives.append(frozenset([-3]))
    missing_negatives.append(frozenset([-3]))
    missing_negatives.append(frozenset([-3]))
    missing_negatives.append(frozenset([3]))

    accuracy = complete_cnfalgo(
        train_positives,
        train_negatives,
        test_positives,
        test_negatives,
        missing_positives,
        missing_negatives,
        cnfalgo_exec_path="Resources/cnfalgo",
        metric="length",
    )
