from sympy import symbols
from tqdm import tqdm
import math
from collections import Counter

# from pyswip import Prolog


def print_input_data_statistics(
    name, pos_freq, neg_freq, target_class, negation, load_top_k, switch_signs
):
    print()
    print("Input Data Statistics:")
    print("\tDataset Name \t\t\t\t\t\t\t\t\t\t\t\t\t\t\t: " + name)
    print(
        "\tAdd absent variables as negative literals in the partial assignments \t: "
        + str(negation)
    )
    if not (bool(load_top_k)):
        print(
            "\tLoad top-k negative partial assignments \t\t\t\t\t\t\t\t: "
            + str(bool(load_top_k))
        )
    else:
        print(
            "\tNumber of negative partial assignments loaded from the top of the file \t: "
            + str(load_top_k)
        )
    print()

    if switch_signs:
        pos_sign = " -ve"
        neg_sign = " +ve"
    else:
        pos_sign = " +ve"
        neg_sign = " -ve"

    print("\tClass\tSign\t%age\tFrequency")
    n = pos_freq + neg_freq
    print(
        "\t"
        + str(target_class[0])
        + "\t\t"
        + pos_sign
        + "\t"
        + str(round(pos_freq * 100 / n))
        + "%\t\t"
        + str(pos_freq)
    )
    print(
        "\t"
        + str(target_class[1])
        + "\t\t"
        + neg_sign
        + "\t"
        + str(round(neg_freq * 100 / n))
        + "%\t\t"
        + str(neg_freq)
    )
    print()


def load_animal_taxonomy():
    a = symbols("a")
    b = symbols("b")
    c = symbols("c")
    d = symbols("d")
    e = symbols("e")
    f = symbols("f")
    g = symbols("g")
    h = symbols("h")
    i = symbols("i")
    j = symbols("j")
    k = symbols("k")
    l = symbols("l")
    m = symbols("m")
    n = symbols("n")

    p1 = frozenset([a, b, ~c, ~d, ~e, ~f, g, ~h, i, j])
    p2 = frozenset([a, ~b, ~c, ~d, ~e, f, g, ~h, i, j])
    p3 = frozenset([a, ~b, c, ~d, ~e, ~f, g, ~h, i, j, ~l, m])
    p4 = frozenset([a, ~b, c, ~d, ~e, ~f, g, ~h, i, j, l, ~m])

    positives = {p1, p2, p3, p4}

    n1 = frozenset([~b, c, ~d, ~e, ~f, g, ~h, i, ~j, k])
    n2 = frozenset([~b, ~c, d, ~e, ~f, ~g, h, i, ~j, ~l, m, n])
    n3 = frozenset([~b, ~c, d, ~e, ~f, ~g, h, i, ~j, l, ~m, n])
    n4 = frozenset([b, ~c, ~d, ~e, ~f, g, ~h, ~i, ~j, k])
    n5 = frozenset([~b, ~c, d, ~e, ~f, g, ~h, i, ~j])
    n6 = frozenset([~b, c, ~d, ~e, ~f, g, ~h, ~i, ~j, ~k, ~l, m])
    n7 = frozenset([~b, ~c, ~d, e, ~f, g, ~h, ~i, ~j, ~k, l, ~m])

    negatives = {n1, n2, n3, n4, n5, n6, n7}

    return positives, negatives


def load_dataset(
    filename,
    total,
    var_range,
    target_class,
    negation=False,
    load_top_k=None,
    switch_signs=False,
):

    f = open("./Data/" + filename, "r")
    name = filename.split(".")[0].split("_")[0]
    name = name[0].upper() + name[1:]

    # positive_input_clauses = set()
    # negative_input_clauses = set()

    positive_input_clauses = []
    negative_input_clauses = []

    pbar = tqdm(f, total=total)
    pbar.set_description("Reading input file for " + name)

    pos_freq = 0
    neg_freq = 0
    for line in pbar:
        row = str(line).replace("\n", "").strip().split(" ")
        partial_assignmemnt = set()
        for j in var_range:
            if str(j) in row[:-1]:
                partial_assignmemnt.add(symbols("v" + str(j)))
            elif negation:
                partial_assignmemnt.add(~symbols("v" + str(j)))

        if (not (switch_signs) and row[-1] == target_class[0]) or (
            switch_signs and row[-1] == target_class[1]
        ):
            negative_input_clauses.append(frozenset(partial_assignmemnt))
            # negative_input_clauses.add(frozenset(partial_assignmemnt))
            neg_freq += 1
            if load_top_k and len(negative_input_clauses) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not (switch_signs) and row[-1] == target_class[1]) or (
            switch_signs and row[-1] == target_class[0]
        ):
            positive_input_clauses.append(frozenset(partial_assignmemnt))
            # positive_input_clauses.add(frozenset(partial_assignmemnt))
            pos_freq += 1
        else:
            print("Row found without target class at the end:\n")
            print(line)

    print_input_data_statistics(
        name, pos_freq, neg_freq, target_class, negation, load_top_k, switch_signs
    )

    inconsistent_clauses = set(positive_input_clauses) & set(negative_input_clauses)
    if len(inconsistent_clauses) != 0:
        print(str(len(inconsistent_clauses)) + " inconsistent clauses found.")
        for i, partial_assignmemnt in enumerate(inconsistent_clauses):
            if i < 10:
                print(i, partial_assignmemnt)
            else:
                break

    redundancies = (
        len(positive_input_clauses)
        + len(negative_input_clauses)
        - len(set(positive_input_clauses))
        - len(set(negative_input_clauses))
    )
    if redundancies != 0:
        print(str(redundancies) + " duplicate found.")

    return (positive_input_clauses, negative_input_clauses)


def load_adult(negation=False, load_top_k=None, switch_signs=False):

    return load_dataset(
        "adult.D97.N48842.C2.num",
        48842,
        list(range(1, 96)),
        ["96", "97"],
        negation,
        load_top_k,
        switch_signs,
    )


def load_breast(negation=False, load_top_k=None, switch_signs=False):

    return load_dataset(
        "breast.D20.N699.C2.num",
        699,
        list(range(1, 19)),
        ["19", "20"],
        negation,
        load_top_k,
        switch_signs,
    )


def load_chess(negation=False, load_top_k=None, switch_signs=False):

    return load_dataset(
        "chess.txt",
        699,
        list(range(1, 74)),
        ["74", "75"],
        negation,
        load_top_k,
        switch_signs,
    )


def load_ionosphere(negation=False, load_top_k=None, switch_signs=False):

    return load_dataset(
        "ionosphere.D157.N351.C2.num",
        351,
        list(range(1, 156)),
        ["156", "157"],
        negation,
        load_top_k,
        switch_signs,
    )


def load_mushroom(negation=False, load_top_k=None, switch_signs=False):
    # schema = "e/p b/c/x/f/k/s f/g/y/s n/b/c/g/r/p/u/e/w/y t/f a/l/c/y/f/m/n/p/s a/d/f/n c/w/d b/n k/n/b/h/g/r/o/p/u/e/w/y e/t b/c/u/e/z/r f/y/k/s f/y/k/s n/b/c/g/o/p/e/w/y n/b/c/g/o/p/e/w/y p/u n/o/w/y n/o/t c/e/f/l/n/p/s/z k/n/b/h/r/o/u/w/y a/c/n/s/v/y g/l/m/p/u/w/d"
    # schema_list = []
    # var_dict = {}
    # var_counter = 0
    # for i, options in enumerate(schema.split(" ")[1:]):
    #     option_list = options.split('/')
    #     schema_list.append(len(option_list))
    #
    #     var_list = []
    #     for j in range(len(option_list)):
    #         var_list.append(str(var_counter+j))
    #     var_counter += len(option_list)
    #
    #     var_dict[i+1] = var_list
    #
    # class_dict = {}
    # var_counter = 0
    # for i, schema in enumerate(schema_list):
    #     for j in range(schema):
    #         class_dict[str(var_counter+j)] = i+1
    #     var_counter += schema
    #
    # print(schema_list)
    # print(var_dict)
    # print(class_dict)

    # f = open("./Data/mushroom.D90.N8124.C2.num", "r")

    # f = open("./Data/mushroom_cp4im.txt", "r")
    # positive_input_clauses = set()
    # negative_input_clauses = set()
    # pbar = tqdm(f, total=8124)
    # pbar.set_description("Reading input file")
    # for line in pbar:
    #     row = str(line)[:-1].split(" ")
    #
    #     clause = set()
    #     for j in range(117):
    #         if str(j) in row:
    #             clause.add(~symbols("v" + str(j)))
    #         elif negation:
    #             clause.add(symbols("v" + str(j)))
    #
    #     # for var in row[:-1]:
    #     #     clause.append(Term('v'+str(var)))
    #     #     var_class = class_dict[var]
    #     #     for other_var in var_dict[var_class]:
    #     #         if other_var != var:
    #     #             a = Term('v' + str(other_var))
    #     #             clause.append(-a)
    #
    #     # print(clause)
    #
    #     if (not (switch_signs) and row[-1] == "0") or (switch_signs and row[-1] == "1"):
    #         negative_input_clauses.add(frozenset(clause))
    #         if load_top_k and len(negative_input_clauses) == load_top_k:
    #             # Top k negative clauses have been loaded already
    #             break
    #     elif (not (switch_signs) and row[-1] == "1") or (
    #         switch_signs and row[-1] == "0"
    #     ):
    #         positive_input_clauses.add(frozenset(clause))
    #
    # print_input_data_statistics(
    #     "Mushroom",
    #     positive_input_clauses,
    #     negative_input_clauses,
    #     ["0", "1"],
    #     negation,
    #     load_top_k,
    #     switch_signs,
    # )

    return load_dataset(
        "mushroom_cp4im.txt",
        8124,
        list(range(0, 117)),
        ["0", "1"],
        negation,
        load_top_k,
        switch_signs,
    )


def load_pima(negation=False, load_top_k=None, switch_signs=False):

    return load_dataset(
        "pima.D38.N768.C2.num",
        768,
        list(range(1, 37)),
        ["37", "38"],
        negation,
        load_top_k,
        switch_signs,
    )


def load_tictactoe(negation=False, load_top_k=None, switch_signs=False):

    return load_dataset(
        "ticTacToe.D29.N958.C2.num",
        958,
        list(range(1, 28)),
        ["28", "29"],
        negation,
        load_top_k,
        switch_signs,
    )


def get_literal_length(theory):
    """
    :param theory:
    :return: the total number of literals in the whole theory
    """
    length = 0
    for clause in theory:
        length += len(clause)

    return length


def get_shannon_entropy_for_clause(clause):
    if len(clause) <= 1:
        return 0

    counts = Counter()
    for d in clause:
        counts[d] += 1

    entropy = 0
    for c in counts.values():
        p = float(c) / len(clause)
        if p > 0.0:
            entropy -= p * math.log(p, 2)

    return entropy


def get_shannon_entropy_for_theory(theory):
    """
    :param theory: a list of clauses which are in the form of frozensets
    :return: the total number of bits required to represent the input theory
    """
    length = 0
    for clause in theory:
        length += get_shannon_entropy_for_clause(clause)
    return length


def get_new_var():
    """
    Invent a new predicate
    :return: A brand new term 'z' followed by an updated counter
    """
    global new_var_counter
    new_var = symbols("z" + str(new_var_counter))
    new_var_counter += 1
    return new_var


def convert_to_clause(partial_assignment):
    """
    Negate the terms in the input frozenset to convert them from a partial assignment to a clause.
    Example:
        input: frozenset({~v1, ~v2, ~v3, ~v4, ~v5, ~v6, ~v7, ~v8})
        output: frozenset({v1, v2, v3, v4, v5, v6, v7, v8})
    :param partial_assignment: Every term in the frozenset is in a conjunction with one another
    :return: clause: Every term in the frozenset is in a disjunction with one another
    """
    clause = set()
    for term in partial_assignment:
        clause.add(~term)
    return frozenset(clause)


def convert_to_theory(partial_assignments):
    """
    Negate the disjunctive set of partial assignments to get a conjunctive set of clauses (theory)
    :param partial_assignments: List/Set of frozensets of terms where each frozenset is a partial assignment
    :return: List of frozensets of terms where each frozenset is a clause
    """
    theory = []
    for pa in partial_assignments:
        theory.append(convert_to_clause(pa))
    return theory


def compress_pairwise(clause1, clause2, lossless=False):
    """
    Compress clause1 and clause2
    :param clause1:
    :param clause2:
    :param lossless: Apply V-operator only if lossless=False
    :return:
        A list of the new clauses obtained after compression
        The decrease in literal length achieved during compression
        A Boolean variable that is:
            True    if a lossless compression step (W-operator, Subsumption) took place
            False   if a lossy compression step (V-operator) took place
            None    if no operator is applied
    """

    global operator_counter
    clause_a = set(clause1 - clause2)
    clause_b = set(clause1 & clause2)
    clause_c = set(clause2 - clause1)

    if len(clause_a) == 0:
        # Apply subsumption operator on (b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        operator_counter["S"] += 1
        return ({clause1}, len(clause_b), True)

    elif len(clause_c) == 0:
        # Apply subsumption operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3)
        operator_counter["S"] += 1
        return ({clause2}, len(clause_b), True)

    elif len(clause_b) == 0:
        # Cannot compress (a1; a2; a3), (c1; c2; c3)
        print("No Overlap Found")
        return ({clause1, clause2}, 0, None)

    elif not lossless and len(clause_a) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a; b1; b2; b3), (c1; c2; c3; -a)
        a = clause_a.pop()
        clause_c.add(~a)
        operator_counter["V"] += 1
        return ({clause1, frozenset(clause_c)}, len(clause_b) - 1, False)

    elif not lossless and len(clause_c) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
        # Return (a1; a2; a3; -c), (b1; b2; b3; c)
        c = clause_c.pop()
        clause_a.add(~c)
        operator_counter["V"] += 1
        return ({frozenset(clause_a), clause2}, len(clause_b) - 1, False)

    elif len(clause_b) < 3:
        # print("Application of W-Operator is infeasible due to overlap of only " + str(len(clause_b)) + " literals.")
        return ({clause1, clause2}, 0, None)

    else:
        # This is the case where len(clause_a) > 1, len(clause_b) > 2, and len(clause_c) > 1.
        # Apply W-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a1; a2; a3; -z), (b1; b2; b3; z), (c1; c2; c3, -z)
        new_var = get_new_var()
        clause_a.add(~new_var)
        clause_b.add(new_var)
        clause_c.add(~new_var)
        operator_counter["W"] += 1
        return (
            {frozenset(clause_a), frozenset(clause_b), frozenset(clause_c)},
            len(clause_b) - 3,
            True,
        )


def check_validity(positives, theory):
    """
    :param positives: A list of positive partial assignments
    :param theory: A SAT Theory on which the validity of positives is to be checked
    :return:
        True    if all positives are valid in the SAT Theory
        False   if at least one of the positives is not True in that theory
    """
    theory_valid = True

    # print("Positives:")
    # print_2d(positives)
    #
    # print("Theory")
    # print_2d(theory)

    # for pos in positives:
    #     pos_invalid = False
    #     for clause in theory:
    #         clause_validy = False
    #         for literal in clause:
    #             if literal in pos:
    #                 clause_validy = True
    #                 break
    #
    #         if not clause_validy:
    #             pos_invalid = True
    #             break
    #
    #     if pos_invalid:
    #         theory_valid = False
    #         break

    return theory_valid


def sort_theory(theory):

    clause_length = []
    for clause in theory:
        clause_length.append(len(clause))

    index = list(range(len(clause_length)))
    index.sort(key=clause_length.__getitem__, reverse=True)
    clause_length = [clause_length[i] for i in index]
    theory = [theory[i] for i in index]

    # for clause, length in zip(theory, clause_length):
    #     print(length, clause)
    # print()

    return theory, clause_length


def print_2d(matrix):
    for row in matrix:
        print(row)
    print()


def get_overlap_matrix(theory):
    """
    Construct a triangular matrix that stores overlap of ith and jth clause at (i,j) position
    :param theory: List of clauses sorted descending by their length
    :param clause_length: List of length of clauses
    :return: A triangular matrix that stores overlap of ith and jth clause at (i,j) position
    """
    overlap_matrix = []
    n = len(theory)
    for i, clause1 in enumerate(theory):
        overlap_matrix.append([0] * n)
        for j, clause2 in enumerate(theory[i + 1 :]):
            overlap_matrix[i][i + j + 1] = len(clause1 & clause2)

    # print_2d(overlap_matrix)

    return overlap_matrix


def select_clauses_1(overlap_matrix):
    """
    Select clauses for next compression step WITH a compression matrix
    :param overlap_matrix: A triangular matrix that stores overlap of ith and jth clause of the theory at (i,j) position
    :return:
        max_overlap_indices: The indices of 2 clauses that have the maximum overlap size
        max_overlap_size: The maximum overlap size
    """

    max_overlap_size = None
    max_overlap_indices = (None, None)
    for i, row in enumerate(overlap_matrix):
        for j, overlap in enumerate(row[i + 1 :]):
            if overlap > max_overlap_size:
                max_overlap_size = overlap
                max_overlap_indices = (i, j)

    return max_overlap_indices, max_overlap_size


def select_clauses_2(theory, clause_length, prev_overlap_size, ignore_indices):
    """
    Select clauses for next compression step WITHOUT a compression matrix
    :param theory: List of clauses sorted descending by their length
    :param clause_length: List of length of clauses
    :param prev_overlap_size: Maximum overlap size in the previous compression step
    :return:
        max_overlap_indices: The indices of 2 clauses that have the maximum overlap size
        max_overlap_size: The maximum overlap size
    """
    overlap_sizes = []
    overlap_indices = []
    max_overlap_size = 0
    max_overlap_indices = None
    end = len(theory)
    for x, clause1 in enumerate(theory):
        for y, clause2 in enumerate(theory[x + 1 : end]):

            if (clause1, clause2) in ignore_indices:
                continue
            # if max_overlap_size > len(clause2):

            # This step may be highly greedy and sub-optimal, but it should make things faster
            if prev_overlap_size and max_overlap_size >= prev_overlap_size:
                break

            if max_overlap_size >= clause_length[x + y + 1]:
                end = x + y + 1
                break

            # TODO: This task could be sped up by storing a triangular matrix of overlaps
            overlap_size = len(clause1 & clause2)
            overlap_indices.append((x, x + y + 1))
            overlap_sizes.append(overlap_size)

            if overlap_size > max_overlap_size:
                max_overlap_size = overlap_size
                max_overlap_indices = (x, x + y + 1)

            # print(str(x), str(x+y+1), str(len(clause_b)))
    return max_overlap_indices, max_overlap_size


def delete_clause(theory, clause_length, overlap_matrix, indices):
    n = len(theory)
    del clause_length[indices[1]]
    del clause_length[indices[0]]
    del theory[indices[1]]
    del theory[indices[0]]

    # print_2d(overlap_matrix)

    del overlap_matrix[indices[1]]
    del overlap_matrix[indices[0]]
    for i in range(n - 2):
        del overlap_matrix[i][indices[1]]
        del overlap_matrix[i][indices[0]]

    # print_2d(overlap_matrix)

    return theory, clause_length, overlap_matrix


def insert_clause(theory, clause_length, overlap_matrix, clause):
    i = int(len(clause_length) / 2)
    start = 0
    end = len(clause_length)
    index_found = False
    while i < len(clause_length):
        if len(clause) <= clause_length[-1]:
            # Insert element at the end of the list
            index = len(clause_length)
            index_found = True
            break
        elif len(clause) >= clause_length[0]:
            # Insert element at the start of the list
            index = 0
            index_found = True
            break
        elif len(clause) <= clause_length[i] and len(clause) >= clause_length[i + 1]:
            # Insert element at/before (i + 1) if
            # clause_length[i] >= len(clause) >= clause_length[i + 1]
            index = i + 1
            index_found = True
            break
        elif len(clause) > clause_length[i]:
            end = i
            i = int((i + start) / 2)
        elif len(clause) < clause_length[i]:
            start = i
            i = int((end + i) / 2)
        else:
            i += 1

    # print_2d(overlap_matrix)

    assert index_found

    for i, clause2 in enumerate(theory):
        if i < index:
            overlap_matrix[i][index:index] = [len(clause & clause2)]
        else:
            overlap_matrix[i][index:index] = [0]

    # print_2d(overlap_matrix)

    new_overlap_row = [0] * (len(theory) + 1)
    for j, clause2 in enumerate(theory[index + 1 :]):
        new_overlap_row[index + j + 1] = len(clause & clause2)

    overlap_matrix[index:index] = [new_overlap_row]

    # print_2d(overlap_matrix)

    theory[index:index] = [clause]
    clause_length[index:index] = [len(clause)]

    return theory, clause_length, overlap_matrix


def mistle(positives, negatives, lossless=False):

    input_length = get_literal_length(negatives)
    print(
        "\nInput Theory:\n\tLiteral Length = "
        + str(input_length)
        + "\n\tNumber of clauses = "
        + str(len(negatives))
    )

    # print_2d(theory)

    # Remove redundant clauses from the theory
    theory = convert_to_theory(set(negatives))
    # print_2d(theory)
    theory, clause_length = sort_theory(theory)

    overlap_matrix = get_overlap_matrix(theory)

    overlap_size = None
    compression_counter = 0
    ignore_clauses = []
    pbar = tqdm(total=int(1.1 * len(theory) + 100))
    pbar.set_description("Compressing Clauses")
    while True:
        prev_overlap_size = overlap_size
        max_overlap_indices, overlap_size = select_clauses_2(
            theory, clause_length, prev_overlap_size, ignore_clauses
        )
        if overlap_size < 2:
            break

        clause1 = theory[max_overlap_indices[0]]
        clause2 = theory[max_overlap_indices[1]]

        # print(
        #     "\nCompressing ("
        #     + str(clause1)
        #     + ") and ("
        #     + str(clause2)
        #     + ") for overlap of "
        #     + str(overlap_size)
        #     + " literals."
        # )
        (compressed_clauses, compression_size, is_lossless) = compress_pairwise(
            clause1, clause2, lossless=lossless
        )
        # print(compression_counter, compressed_clauses, compression_size, is_lossless)
        compression_counter += 1

        if len(compressed_clauses) == 2 and compression_size == 0:
            # Theory cannot be compressed any further
            # ignore_clauses.append((clause1, clause2))
            # continue
            break
        elif is_lossless or (
            is_lossless is False and check_validity(positives, theory)
        ):
            # Continue compressing the theory
            # Delete the clauses used for last compression step
            theory, clause_length, overlap_matrix = delete_clause(
                theory, clause_length, overlap_matrix, max_overlap_indices
            )

            # Insert clauses from compressed_clauses at appropriate lengths so that the theory remains sorted
            # theory |= compressed_clauses
            for clause in compressed_clauses:
                theory, clause_length, overlap_matrix = insert_clause(
                    theory, clause_length, overlap_matrix, clause
                )

            # ignore_clauses = []

        # print_2d(theory)
        # pbar.update(1)

    global new_var_counter, operator_counter
    output_length = get_literal_length(theory)
    print(
        "\nResultant Theory:\n\tLiteral Length = "
        + str(output_length)
        + "\n\tNumber of clauses = "
        + str(len(theory))
        + "\n\tNumber of invented predicates = "
        + str(new_var_counter - 1)
        + "\n\t% Compressed Literals = "
        + str(round(output_length * 100 / input_length, 2))
        + "%"
        + "\n\tOperator Count = "
        + str(operator_counter)
        + "\n\tOverlap Matrix = "
    )
    # print_2d(overlap_matrix)
    # print()
    # print_2d(theory)

    return theory


new_var_counter = 1
operator_counter = {"W": 0, "V": 0, "S": 0}
# positives, negatives = load_animal_taxonomy()
# theory = mistle(positives, negatives, lossless=False)
# theory = mistle(negatives, positives, lossless=False)
# _, input_clauses = load_mushroom(negation = True, load_top_k = None)
# _, input_clauses = load_adult()
# _, input_clauses = load_breast()
# _, input_clauses = load_mushroom()
_, input_clauses = load_ionosphere()
theory = mistle(None, input_clauses, lossless=False)

# _, input_clauses = load_pima()
# _, input_clauses = load_tictactoe()
# theory = mistle(input_clauses)

# _, input_clauses = load_breast()
# theory = mistle(input_clauses)
#
# _, input_clauses = load_breast(switch_signs=True)
# theory = mistle(input_clauses)
#
# _, input_clauses = load_breast(negation=True)
# theory = mistle(input_clauses)
#
# _, input_clauses = load_breast(negation=True, switch_signs=True)
# theory = mistle(input_clauses)


# _, input_clauses = load_chess()
# theory = mistle(input_clauses)
#
# _, input_clauses = load_chess(switch_signs=True)
# theory = mistle(input_clauses)

# _, input_clauses = load_chess(negation=True)
# theory = mistle(input_clauses)
#
# _, input_clauses = load_chess(negation=True, switch_signs=True)
# theory = mistle(input_clauses)


# _, input_clauses = load_pima()
# theory = mistle(input_clauses)

# _, input_clauses = load_pima(switch_signs=True)
# theory = mistle(input_clauses)
#
# _, input_clauses = load_pima(negation=True)
# theory = mistle(input_clauses)
#
# _, input_clauses = load_pima(negation=True, switch_signs=True)
# theory = mistle(input_clauses)
#
#
# _, input_clauses = load_tictactoe()
# theory = mistle(input_clauses)
#
# _, input_clauses = load_tictactoe(switch_signs=True)
# theory = mistle(input_clauses)
#
# _, input_clauses = load_tictactoe(negation=True)
# theory = mistle(input_clauses)
#
# _, input_clauses = load_tictactoe(negation=True, switch_signs=True)
# theory = mistle(input_clauses)
