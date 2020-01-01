from tqdm import tqdm
import math
from collections import Counter
from copy import copy
from time import time
import numpy as np

# from pysmt.shortcuts import Symbol, And, Or, Not, is_sat
# from z3 import *
from pycosat import solve

np.set_printoptions(linewidth=150)


def print_input_data_statistics(
    name,
    positive_input_clauses,
    negative_input_clauses,
    target_class,
    negation,
    load_top_k,
    switch_signs,
):
    pos_freq = len(positive_input_clauses)
    neg_freq = len(negative_input_clauses)

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

    inconsistent_clauses = set(positive_input_clauses) & set(negative_input_clauses)
    print(
        "\tNumber of inconsistent clauses found \t\t\t\t\t\t\t\t\t: "
        + str(len(inconsistent_clauses))
    )

    pos_redundancies = len(positive_input_clauses) - len(set(positive_input_clauses))
    print(
        "\tNumber of positive redundancies found \t\t\t\t\t\t\t\t\t: "
        + str(pos_redundancies)
    )

    neg_redundancies = len(negative_input_clauses) - len(set(negative_input_clauses))
    print(
        "\tNumber of negative redundancies found \t\t\t\t\t\t\t\t\t: "
        + str(neg_redundancies)
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


def load_animal_taxonomy(switch_signs=False):
    a = 1
    b = 2
    c = 3
    d = 4
    e = 5
    f = 6
    g = 7
    h = 8
    i = 9
    j = 10
    k = 11
    l = 12
    m = 13
    n = 14

    global new_var_counter
    new_var_counter = 15

    p1 = frozenset([a, b, -c, -d, -e, -f, g, -h, i, j])
    p2 = frozenset([a, -b, -c, -d, -e, f, g, -h, i, j])
    p3 = frozenset([a, -b, c, -d, -e, -f, g, -h, i, j, -l, m])
    p4 = frozenset([a, -b, c, -d, -e, -f, g, -h, i, j, l, -m])

    positives = {p1, p2, p3, p4}

    n1 = frozenset([-b, c, -d, -e, -f, g, -h, i, -j, k])
    n2 = frozenset([-b, -c, d, -e, -f, -g, h, i, -j, -l, m, n])
    n3 = frozenset([-b, -c, d, -e, -f, -g, h, i, -j, l, -m, n])
    n4 = frozenset([b, -c, -d, -e, -f, g, -h, -i, -j, k])
    n5 = frozenset([-b, -c, d, -e, -f, g, -h, i, -j])
    n6 = frozenset([-b, c, -d, -e, -f, g, -h, -i, -j, -k, -l, m])
    n7 = frozenset([-b, -c, -d, e, -f, g, -h, -i, -j, -k, l, -m])

    negatives = {n1, n2, n3, n4, n5, n6, n7}

    print_input_data_statistics(
        "Animal Taxonomy",
        positives,
        negatives,
        target_class=["B", "NB"],
        negation=False,
        load_top_k=False,
        switch_signs=False,
    )
    if switch_signs:
        return negatives, positives
    else:
        return positives, negatives


def load_dataset(
    filename,
    total,
    var_range,
    target_class,
    negation=False,
    load_top_k=None,
    switch_signs=False,
    num_vars=0,
    load_tqdm=True,
):
    global new_var_counter
    new_var_counter = num_vars + 1

    f = open("./Data/" + filename, "r")
    name = filename.split(".")[0].split("_")[0]
    name = name[0].upper() + name[1:]

    # positive_input_clauses = set()
    # negative_input_clauses = set()

    positive_pas = []
    negative_pas = []

    if load_tqdm:
        pbar = tqdm(f, total=total)
        pbar.set_description("Reading input file for " + name)

    pos_freq = 0
    neg_freq = 0
    for line in f:
        row = str(line).replace("\n", "").strip().split(" ")
        partial_assignmemnt = set()
        for i, j in enumerate(var_range):
            if str(j) in row[:-1]:
                partial_assignmemnt.add(i + 1)
            elif negation:
                # Using closed world assumption to register absent variables as if they are false.
                partial_assignmemnt.add(-(i + 1))

        if (not (switch_signs) and row[-1] == target_class[0]) or (
            switch_signs and row[-1] == target_class[1]
        ):
            negative_pas.append(frozenset(partial_assignmemnt))
            neg_freq += 1
            if load_top_k and len(negative_pas) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not (switch_signs) and row[-1] == target_class[1]) or (
            switch_signs and row[-1] == target_class[0]
        ):
            positive_pas.append(frozenset(partial_assignmemnt))
            pos_freq += 1
        else:
            print("Row found without target class at the end:\n")
            print(line)

        if load_tqdm:
            pbar.update(1)

    print_input_data_statistics(
        name,
        positive_pas,
        negative_pas,
        target_class,
        negation,
        load_top_k,
        switch_signs,
    )

    # # Remove redundant partial assignments
    # positive_pas = set(positive_pas)
    # negative_pas = set(negative_pas)
    #
    # # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
    # inconsistent_pas = positive_pas & negative_pas
    # positive_pas = positive_pas - inconsistent_pas
    # negative_pas = negative_pas - inconsistent_pas
    #
    # input_literal_length = get_literal_length(negative_pas)
    # input_bit_length = get_bit_length_for_theory(negative_pas)
    #
    # print(
    #     "\nInput Theory (without redundancies and inconsistencies):\n\tLiteral Length = "
    #     + str(input_literal_length)
    #     + "\n\tBit Length = "
    #     + str(input_bit_length)
    #     + "\n\tNumber of clauses = "
    #     + str(len(negative_pas))
    # )
    return (positive_pas, negative_pas)

    # # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
    # new_positive_pas = []
    # for pa in positive_pas:
    #     if pa not in negative_pas:
    #         new_positive_pas.append(pa)
    #
    # new_negative_pas = []
    # for pa in negative_pas:
    #     if pa not in positive_pas:
    #         new_negative_pas.append(pa)
    #
    # input_literal_length = get_literal_length(new_negative_pas)
    # input_bit_length = get_bit_length_for_theory(new_negative_pas)
    #
    # print(
    #     "\nInput Theory (only without inconsistencies):\n\tLiteral Length = "
    #     + str(input_literal_length)
    #     + "\n\tBit Length = "
    #     + str(input_bit_length)
    #     + "\n\tNumber of clauses = "
    #     + str(len(negative_pas))
    # )
    #
    # return (new_positive_pas, new_negative_pas)


def load_adult(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "adult.D97.N48842.C2.num",
        48842,
        list(range(1, 96)),
        ["96", "97"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=95,
        load_tqdm=load_tqdm,
    )


def load_breast(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "breast.D20.N699.C2.num",
        699,
        list(range(1, 19)),
        ["19", "20"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=18,
        load_tqdm=load_tqdm,
    )


def load_chess(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "chess.txt",
        3196,
        list(range(1, 74)),
        ["74", "75"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=73,
        load_tqdm=load_tqdm,
    )


def load_ionosphere(
    negation=False, load_top_k=None, switch_signs=False, load_tqdm=True
):
    return load_dataset(
        "ionosphere.D157.N351.C2.num",
        351,
        list(range(1, 156)),
        ["156", "157"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=155,
        load_tqdm=load_tqdm,
    )


def load_mushroom(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "mushroom_cp4im.txt",
        8124,
        list(range(0, 117)),
        ["0", "1"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=116,
        load_tqdm=load_tqdm,
    )


def load_pima(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):
    return load_dataset(
        "pima.D38.N768.C2.num",
        768,
        list(range(1, 37)),
        ["37", "38"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=36,
        load_tqdm=load_tqdm,
    )


def load_tictactoe(negation=False, load_top_k=None, switch_signs=False, load_tqdm=True):

    return load_dataset(
        "ticTacToe.D29.N958.C2.num",
        958,
        list(range(1, 28)),
        ["28", "29"],
        negation,
        load_top_k,
        switch_signs,
        num_vars=27,
        load_tqdm=load_tqdm,
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


def get_shannon_entropy_for_list(clause):
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


def get_shannon_entropy_elementwise(theory):
    """
    :param theory: a list of clauses which are in the form of frozensets
    :return: the total number of bits required to represent the input theory
    """
    length = 0
    bit_lengths = []
    for clause in theory:
        bl = get_shannon_entropy_for_list(clause)
        length += bl
        bit_lengths.append(bl)

    print(bit_lengths)

    return length


def get_bit_length_for_theory(theory):
    """
    :param theory: a list of clauses which are in the form of frozensets
    :return: the total number of bits required to represent the input theory
    """

    counts = Counter()
    total_literals = 0
    for clause in theory:
        for literal in clause:
            counts[literal] += 1
            total_literals += 1

    entropy = 0
    for v in counts.values():
        # for k, v in counts.items():
        # print(k, ":\t", v)
        p = float(v) / total_literals
        if p > 0.0:
            entropy -= math.log(p, 2)

    return entropy


def get_new_var():
    """
    Invent a new predicate
    :return: A brand new term 'z' followed by an updated counter
    """
    global new_var_counter
    new_var = new_var_counter
    new_var_counter += 1
    return new_var


def convert_to_clause(partial_assignment):
    """
    Negate the terms in the input frozenset to convert them from a partial assignment to a clause.
    Example:
        input: frozenset({-1*v1, -1*v2, -1*v3, -1*v4, -1*v5, -1*v6, -1*v7, -1*v8})
        output: frozenset({v1, v2, v3, v4, v5, v6, v7, v8})
    :param partial_assignment: Every term in the frozenset is in a conjunction with one another
    :return: clause: Every term in the frozenset is in a disjunction with one another
    """
    clause = set()
    for term in partial_assignment:
        clause.add(-term)
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


def get_subclauses(clause1, clause2):
    clause_a = copy(set(clause1 - clause2))
    clause_b = copy(set(clause1 & clause2))
    clause_c = copy(set(clause2 - clause1))
    return clause_a, clause_b, clause_c


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

    global operator_counter, invented_predicate_definition
    clause_a, clause_b, clause_c = get_subclauses(clause1, clause2)

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
        print_1d(clause1)
        print_1d(clause2)
        print("No Overlap Found")
        return ({clause1, clause2}, 0, None)

    elif not lossless and len(clause_a) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a; b1; b2; b3), (c1; c2; c3; -a)
        a = clause_a.pop()
        clause_c.add(-a)
        operator_counter["V"] += 1
        return ({clause1, frozenset(clause_c)}, len(clause_b) - 1, False)

    elif not lossless and len(clause_c) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
        # Return (a1; a2; a3; -c), (b1; b2; b3; c)
        c = clause_c.pop()
        clause_a.add(-c)
        operator_counter["V"] += 1
        return ({frozenset(clause_a), clause2}, len(clause_b) - 1, False)

    elif len(clause_b) < 3:
        # print("Application of W-Operator is infeasible due to overlap of only " + str(len(clause_b)) + " literals.")
        return ({clause1, clause2}, 0, None)

    else:
        # This is the case where len(clause_a) > 1, len(clause_b) > 2, and len(clause_c) > 1.
        # Apply W-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a1; a2; a3; -z), (b1; b2; b3; z), (c1; c2; c3, -z)
        if clause_b not in invented_predicate_definition.items():
            # Invent a new predicate if the definition of a predicate is new.
            new_var = get_new_var()
            clause_a.add(-new_var)
            invented_predicate_definition[new_var] = copy(clause_b)
            clause_b.add(new_var)
            clause_c.add(-new_var)
        else:
            for var, clause in invented_predicate_definition.items():
                if clause == clause_b:
                    clause_a.add(-var)
                    clause_b.add(var)
                    clause_c.add(-var)

        operator_counter["W"] += 1
        return (
            {frozenset(clause_a), frozenset(clause_b), frozenset(clause_c)},
            len(clause_b) - 3,
            True,
        )


def check_subset(pa_set, x):

    for pa in pa_set:
        x_found = True
        for literal in x:
            if literal not in pa:
                x_found = False
                break

        if x_found:
            return True

    return False


def substitute_pa(pa):
    global invented_predicate_definition

    # new_pa = set()
    # for literal in pa:
    #
    #     if literal < 0:
    #         base_literal = -literal
    #     else:
    #         base_literal = literal
    #
    #     if base_literal in invented_predicate_definition:
    #         for l in invented_predicate_definition[base_literal]:
    #             if literal > 0:
    #                 if l in new_pa:
    #                     return False
    #                 new_pa.add(-l)
    #             else:
    #                 if -l in new_pa:
    #                     return False
    #                 new_pa.add(l)
    #     else:
    #         new_pa.add(literal)
    #
    # return new_pa

    new_pas = [set()]
    for literal in pa:

        if literal < 0:
            base_literal = -literal
        else:
            base_literal = literal

        if literal > 0:
            if base_literal in invented_predicate_definition:
                for l in invented_predicate_definition[base_literal]:

                    for i in range(len(new_pas)):
                        if l in new_pas[i]:
                            new_pas[i] = {"False"}
                        elif "False" not in new_pas[i]:
                            new_pas[i].add(-l)
            else:
                for i in range(len(new_pas)):
                    if -literal in new_pas[i]:
                        new_pas[i] = {"False"}
                    elif "False" not in new_pas[i]:
                        new_pas[i].add(literal)
        else:
            if base_literal in invented_predicate_definition:
                # Recurse on all the literals present in the definition of the invented predicate
                new_pas = new_pas * len(invented_predicate_definition[base_literal])

                for i, l in enumerate(invented_predicate_definition[base_literal]):
                    if -l in new_pas[i]:
                        new_pas[i] = {"False"}
                    elif "False" not in new_pas[i]:
                        new_pas[i].add(l)

            else:
                for i in range(len(new_pas)):
                    if -literal in new_pas[i]:
                        new_pas[i] = {"False"}
                    elif "False" not in new_pas[i]:
                        new_pas[i].add(literal)

    pruned_pas = []
    for new_pa in new_pas:
        if "False" in new_pa:
            continue
        else:
            pruned_pas.append(new_pa)

    return pruned_pas


def substitute_clause(clause):
    # TODO: Why do we not substitute inveted predicates that are in the definition of other invented predicates?
    global invented_predicate_definition

    new_clauses = [set()]
    for literal in clause:

        if literal < 0:
            base_literal = -literal
        else:
            base_literal = literal

        if literal > 0:
            if base_literal in invented_predicate_definition:
                # Recurse on all the literals present in the definition of the invented predicate
                new_clauses = new_clauses * len(
                    invented_predicate_definition[base_literal]
                )

                for i, l in enumerate(invented_predicate_definition[base_literal]):
                    if l in new_clauses[i]:
                        new_clauses[i] = {"True"}
                    elif "True" not in new_clauses[i]:
                        new_clauses[i].add(-l)
            else:
                for i in range(len(new_clauses)):
                    if -literal in new_clauses[i]:
                        new_clauses[i] = {"True"}
                    elif "True" not in new_clauses[i]:
                        new_clauses[i].add(literal)
        else:
            if base_literal in invented_predicate_definition:
                for l in invented_predicate_definition[base_literal]:

                    for i in range(len(new_clauses)):
                        if -l in new_clauses[i]:
                            new_clauses[i] = {"True"}
                        elif "True" not in new_clauses[i]:
                            new_clauses[i].add(l)
            else:
                for i in range(len(new_clauses)):
                    if -literal in new_clauses[i]:
                        new_clauses[i] = {"True"}
                    elif "True" not in new_clauses[i]:
                        new_clauses[i].add(literal)

    pruned_clauses = []
    for new_clause in new_clauses:
        if "True" in new_clause:
            continue
        else:
            pruned_clauses.append(new_clause)

    return pruned_clauses


def check_clause_validity(positives, negatives, clause1, clause2):
    """
    :param positives: A list of positive partial assignments
    :param negatives: A list of negative partial assignments
    :param clause1, clause2: 2 clauses for V-operator
    :return:
        True    if all positives are valid in the SAT Theory
        False   if at least one of the positives is not True in that theory
    """

    clause_a, clause_b, clause_c = get_subclauses(clause1, clause2)
    # pos_loss_pa = copy(clause_b)

    if (len(clause_a) == 1 or len(clause_c) == 1) and len(clause_b) > 1:

        if len(clause_a) == 1:
            # V-operator is applied on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
            # To get (a; b1; b2; b3), (c1; c2; c3; -a)

            # for c in clause_c:
            #     pos_loss_pa.add(-c)
            #
            # a = copy(clause_a).pop()
            # pos_loss_pa.add(a)

            for pos_pa in positives:
                if (
                    check_pa_satisfiability(pos_pa, [clause_a])
                    and check_pa_satisfiability(pos_pa, [clause_b])
                    and not check_pa_satisfiability(pos_pa, [clause_c])
                ):
                    # Loss of a positive partial assignment encountered.
                    return False

            for neg_pa in negatives:
                if (
                    not check_pa_satisfiability(neg_pa, [clause_a])
                    and not check_pa_satisfiability(neg_pa, [clause_b])
                    and check_pa_satisfiability(neg_pa, [clause_c])
                ):
                    # Loss of a negative partial assignment encountered.
                    return False

        elif len(clause_c) == 1:
            # V-operator is applied on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
            # To get (a1; a2; a3; -c), (b1; b2; b3; c)

            # for a in clause_a:
            #     pos_loss_pa.add(-a)
            #
            # c = copy(clause_c).pop()
            # pos_loss_pa.add(c)

            for pos_pa in positives:
                if (
                    not check_pa_satisfiability(pos_pa, [clause_a])
                    and check_pa_satisfiability(pos_pa, [clause_b])
                    and check_pa_satisfiability(pos_pa, [clause_c])
                ):
                    # Loss of a positive partial assignment encountered.
                    return False

            for neg_pa in negatives:
                if (
                    check_pa_satisfiability(neg_pa, [clause_a])
                    and not check_pa_satisfiability(neg_pa, [clause_b])
                    and not check_pa_satisfiability(neg_pa, [clause_c])
                ):
                    # Loss of a negative partial assignment encountered.
                    return False

        # pos_substituted_pas = substitute_pa(pos_loss_pa)
        #
        # if len(pos_substituted_pas) == 0:
        #     # pos_loss_pa is inconsistent; So it cannot be in the data
        #     return True
        #
        # for pos_substituted_pa in pos_substituted_pas:
        #     neg_substituted_pa = set()
        #     for literal in pos_substituted_pa:
        #         if literal == "True" or literal == "False":
        #             print(
        #                 "True/False encountered in the partial assignment",
        #                 str(pos_substituted_pa),
        #             )
        #         neg_substituted_pa.add(-literal)
        #
        #     pos_substituted_pa = frozenset(pos_substituted_pa)
        #     neg_substituted_pa = frozenset(neg_substituted_pa)
        #
        #     if check_subset(positives, pos_substituted_pa) or check_subset(
        #         negatives, neg_substituted_pa
        #     ):
        #         return False

        return True

    else:
        return True


# def convert_to_formula(theory):
#     conjuncts = set()
#     for clause in theory:
#         substituted_clauses = substitute_clause(clause)
#
#         for sub_clause in substituted_clauses:
#             or_list = set()
#             for literal in sub_clause:
#                 if literal > 0:
#                     or_list.add(Symbol(str(literal)))
#                 else:
#                     or_list.add(Not(Symbol(str(literal))))
#             conjuncts.add(Or(list(or_list)))
#     return conjuncts
#
#
# def check_pa_satisfiability1(pa, theory, sign=None):
#     conjuncts = convert_to_formula(theory)
#     for literal in pa:
#         if literal > 0:
#             conjuncts.add(Symbol(str(literal)))
#         else:
#             conjuncts.add(Not(Symbol(str(literal))))
#
#     formula = And(list(conjuncts))
#     return is_sat(formula)


def check_pa_satisfiability(pa, theory):

    theory_cnf = [list(clause) for clause in theory]

    cnf = theory_cnf + [(a,) for a in pa]

    return not (solve(cnf) == "UNSAT")


def check_pa_validity(pa, theory, sign=None):
    """
    1. A partial assignment is invalid wrt a theory if and only if
            there exists a clause 'c' in theory where the given pa is invalid.

    2. A partial assignment is invalid in a clause if and only if
            the negations of all the literals in the clause are present in the pa, i.e.,
            Forall 'a' in clause, -a lies in the partial assignment.

    :param pa:
    :param theory:
    :param sign:
    :return:
    """

    pa_valid = True
    for i, clause in enumerate(theory):
        substituted_clauses = substitute_clause(clause)

        for sub_clause in substituted_clauses:
            clause_valid = False
            for literal in sub_clause:
                if -literal not in pa:
                    clause_valid = True
                    break

            if sign == "+" and not clause_valid:
                print("Clause violated by partial assignment")
                print("Clause", end="\t")
                print_1d(clause)
                print("Substituted Clause", end="\t")
                print_1d(sub_clause)
                print("Partial Assignment", end="\t")
                print_1d(pa)
                a = 1

            if not clause_valid:
                pa_valid = False
                break

    if sign == "-" and pa_valid:
        print("Clause violated by partial assignment")
        print("Clause", end="\t")
        print_1d(clause)
        print("Substituted Clause", end="\t")
        print_1d(sub_clause)
        print("Partial Assignment", end="\t")
        print_1d(pa)
        a = 1

    return pa_valid


def count_violations(positives, negatives, theory, sign=None, print_violations=True):

    start_time = time()
    pos_counter = 0
    neg_counter = 0
    violated_pos = []
    violated_neg = []

    if sign is None or sign == "+":

        for pos in positives:
            # if not check_pa_validity(pos, theory, "+"):
            if not check_pa_satisfiability(pos, theory):
                violated_pos.append(pos)
                pos_counter += 1

        if pos_counter > 0:
            print(
                pos_counter,
                " violations of positive partial assignments found for the learned theory.",
            )
            if print_violations:
                print_2d(violated_pos)
                print()

    elif sign is None or sign == "-":

        for neg in negatives:
            # if check_pa_validity(neg, theory, "-"):
            if check_pa_satisfiability(neg, theory):
                violated_neg.append(neg)
                neg_counter += 1

        if neg_counter > 0:
            print(
                neg_counter,
                " violations of negative partial assignments found for the learned theory.",
            )
            if print_violations:
                print_2d(violated_neg)
                print()

    print(
        "Time taken in counting violations (Sign = "
        + str(sign)
        + ") = "
        + str(time() - start_time)
        + " seconds"
    )
    return (pos_counter + neg_counter, violated_pos, violated_neg)


def sort_theory(theory):

    clause_length = []
    for clause in theory:
        clause_length.append(len(clause))
    clause_length = np.array(clause_length)
    index = clause_length.argsort()[::-1]
    clause_length = clause_length[index]

    # index = list(range(len(clause_length)))
    # index.sort(key=clause_length.__getitem__, reverse=True)
    # clause_length = [clause_length[i] for i in index]
    theory = [theory[i] for i in index]

    # print_2d(theory)

    return theory, clause_length


def print_1d(frozen_set):
    l = list(frozen_set)
    abs_l = [abs(i) for i in l]
    print(np.array([x for _, x in sorted(zip(abs_l, l))]))


def print_2d(matrix):
    for row in matrix:
        if isinstance(row, np.ndarray):
            print(row)
        elif isinstance(row, frozenset):
            print_1d(row)
        else:
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

    return np.array(overlap_matrix)


def get_overlap_list(theory):
    """
    Construct a list that stores overlap of ith and jth clause at (i*n + j)th position
    :param theory: List of clauses sorted descending by their length
    :param clause_length: List of length of clauses
    :return: A list that stores overlap of ith and jth clause at (i*n + j)th position
    """
    overlap_list = []
    for i, clause1 in enumerate(theory):
        for j, clause2 in enumerate(theory[i + 1 :]):
            overlap_list.append((i, i + j + 1, len(clause1 & clause2)))

    # print(overlap_list)

    overlap_list = np.array(overlap_list)
    index = np.argsort(overlap_list[:, 2])[::-1]
    sorted_overlap_list = overlap_list[index]

    # print(sorted_overlap_list)

    return sorted_overlap_list


def select_clauses(overlap_list):
    return (overlap_list[0][0], overlap_list[0][1]), overlap_list[0][2]


def select_clauses_1(
    overlap_matrix, overlap_list, overlap_indices, search_index, prev_overlap_size
):
    """
    Select clauses for next compression step WITH a compression matrix
    :param overlap_matrix: A triangular matrix that stores overlap of ith and jth clause of the theory at (i,j) position
    :return:
        max_overlap_indices: The indices of 2 clauses that have the maximum overlap size
        max_overlap_size: The maximum overlap size
    """

    # max_overlap_size = 0
    # max_overlap_indices = (0, 0)
    # global row_pointer, col_pointer
    # for i, row in enumerate(overlap_matrix[row_pointer:]):
    #     for j, overlap in enumerate(row[max(col_pointer, row_pointer + i + 1) :]):
    #         if overlap > max_overlap_size:
    #             max_overlap_size = overlap
    #             max_overlap_indices = (
    #                 row_pointer + i,
    #                 max(col_pointer, row_pointer + i + 1) + j,
    #             )
    #             if prev_overlap_size and max_overlap_size >= prev_overlap_size:
    #                 break
    #
    # (row_pointer, col_pointer) = max_overlap_indices

    # return max_overlap_indices, max_overlap_size

    return overlap_indices[0], overlap_list[0]


# def select_clauses_2(theory, clause_length, prev_overlap_size, ignore_indices):
def select_clauses_2(theory, clause_length, prev_overlap_size):
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

            # if (clause1, clause2) in ignore_indices:
            #     continue
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


def delete_clause_test(theory, clause_length, overlap_list, indices):

    assert indices[0] < indices[1]

    clause_length = np.delete(clause_length, indices)
    del theory[indices[1]]
    del theory[indices[0]]

    return theory, clause_length, overlap_list


def delete_clause(theory, clause_length, overlap_list, indices):

    assert indices[0] < indices[1]

    clause_length = np.delete(clause_length, indices)
    del theory[indices[1]]
    del theory[indices[0]]

    # print(overlap_list)

    remove_indices = []
    for k, overlap_triplet in enumerate(overlap_list):
        i, j, overlap = overlap_triplet
        if i == indices[0] or i == indices[1] or j == indices[0] or j == indices[1]:
            remove_indices.append(k)
            continue

        if indices[0] < i and i < indices[1]:
            overlap_list[k][0] -= 1
        elif indices[1] < i:
            overlap_list[k][0] -= 2

        if indices[0] < j and j < indices[1]:
            overlap_list[k][1] -= 1
        elif indices[1] < j:
            overlap_list[k][1] -= 2

    overlap_list = np.delete(overlap_list, remove_indices, axis=0)

    # print(overlap_list)

    return theory, clause_length, overlap_list


def delete_clause_1(theory, clause_length, overlap_matrix, indices):
    n = len(theory)
    clause_length = np.delete(clause_length, indices)
    # del clause_length[indices[1]]
    # del clause_length[indices[0]]
    del theory[indices[1]]
    del theory[indices[0]]

    # print_2d(overlap_matrix)

    overlap_matrix = np.delete(overlap_matrix, indices[1], 0)
    overlap_matrix = np.delete(overlap_matrix, indices[0], 0)
    overlap_matrix = np.delete(overlap_matrix, indices[1], 1)
    overlap_matrix = np.delete(overlap_matrix, indices[0], 1)

    # del overlap_matrix[indices[1]]
    # del overlap_matrix[indices[0]]
    # for i in range(n - 2):
    #     del overlap_matrix[i][indices[1]]
    #     del overlap_matrix[i][indices[0]]

    # print_2d(overlap_matrix)

    return theory, clause_length, overlap_matrix


def insert_clause(theory, clause_length, overlap_list, clause):

    clause_index = len(clause_length)
    clause_length = np.append(clause_length, len(clause))

    for i, old_clause in enumerate(theory):
        overlap = len(clause & old_clause)

        insert_index = np.searchsorted(-overlap_list[:, 2], -overlap, side="right")
        overlap_list = np.insert(
            overlap_list, insert_index, np.array((i, clause_index, overlap)), 0
        )

    theory.append(clause)

    return theory, clause_length, overlap_list, clause_index


def insert_clause_1(theory, clause_length, overlap_matrix, clause):
    # i = int(len(clause_length) / 2)
    # start = 0
    # end = len(clause_length)
    # index_found = False
    # while i < len(clause_length):
    #     if len(clause) <= clause_length[-1]:
    #         # Insert element at the end of the list
    #         index = len(clause_length)
    #         index_found = True
    #         break
    #     elif len(clause) >= clause_length[0]:
    #         # Insert element at the start of the list
    #         index = 0
    #         index_found = True
    #         break
    #     elif len(clause) <= clause_length[i] and len(clause) >= clause_length[i + 1]:
    #         # Insert element at/before (i + 1) if
    #         # clause_length[i] >= len(clause) >= clause_length[i + 1]
    #         index = i + 1
    #         index_found = True
    #         break
    #     elif len(clause) > clause_length[i]:
    #         end = i
    #         i = int((i + start) / 2)
    #     elif len(clause) < clause_length[i]:
    #         start = i
    #         i = int((end + i) / 2)
    #     else:
    #         i += 1
    # assert index_found

    index = np.searchsorted(-clause_length, -len(clause), side="right")

    print("Inserting clause at index = " + str(index))

    # Insert zero rows and columns in overlap_matrix at index
    overlap_matrix = np.insert(overlap_matrix, index, np.zeros(len(overlap_matrix)), 0)
    overlap_matrix = np.insert(overlap_matrix, index, np.zeros(len(overlap_matrix)), 1)
    # print_2d(overlap_matrix)
    theory[index:index] = [clause]

    for i, clause2 in enumerate(theory):
        if i == index:
            continue
        overlap = len(clause & clause2)
        if i < index:
            overlap_matrix[i][index] = overlap
        else:
            overlap_matrix[index][i] = overlap

    # print_2d(overlap_matrix)

    # new_overlap_row = [0] * (len(theory) + 1)
    # for j, clause2 in enumerate(theory[index + 1 :]):
    #     new_overlap_row[index + j + 1] = len(clause & clause2)
    #
    # overlap_matrix[index:index] = [new_overlap_row]

    # clause_length[index:index] = [len(clause)]
    clause_length = np.insert(clause_length, index, len(clause))

    # print_2d(theory)
    # print_1d(clause_length)

    return theory, clause_length, overlap_matrix, index


def mistle(positives, negatives, lossless=False, description_measure="ll"):

    start_time = time()

    global operator_counter, new_var_counter
    operator_counter = {"W": 0, "V": 0, "S": 0}

    input_literal_length = get_literal_length(negatives)
    input_bit_length = get_bit_length_for_theory(negatives)

    print(
        "\nInput Theory (with redundancies and inconsistencies):\n\tLiteral Length = "
        + str(input_literal_length)
        + "\n\tBit Length = "
        + str(input_bit_length)
        + "\n\tNumber of clauses = "
        + str(len(negatives))
    )

    # Remove redundant partial assignments
    positives = set(positives)
    negatives = set(negatives)

    # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
    inconsistent_pas = positives & negatives
    positives = positives - inconsistent_pas
    negatives = negatives - inconsistent_pas

    print(
        "\nInput Theory (without redundancies and inconsistencies):\n\tLiteral Length = "
        + str(get_literal_length(negatives))
        + "\n\tBit Length = "
        + str(get_bit_length_for_theory(negatives))
        + "\n\tNumber of clauses = "
        + str(len(negatives))
    )

    if description_measure == "ll":
        input_length = input_literal_length
    elif description_measure == "se":
        input_length = input_bit_length

    # Convert the set of -ve PAs to a theory
    theory = convert_to_theory(negatives)

    initial_violations, positive_violations, negative_violations = count_violations(
        positives, negatives, theory, sign="+", print_violations=False
    )

    if len(positive_violations) > 0:
        # Ignore Positive Violations while calculating the loss
        positives = copy(positives - set(positive_violations))

        # initial_violations, positive_violations, negative_violations = count_violations(
        #     positives, negatives, theory, sign="+"
        # )

    # print_2d(theory)
    theory, clause_length = sort_theory(theory)

    overlap_matrix = get_overlap_matrix(theory)
    overlap_list = get_overlap_list(theory)

    overlap_size = None
    # search_index = 0
    compression_counter = 0
    # ignore_clauses = []
    pbar = tqdm(total=int(1.1 * len(theory) + 100))
    pbar.set_description("Compressing Clauses")
    while True:
        prev_overlap_size = overlap_size
        # max_overlap_indices, overlap_size = select_clauses_2(
        #     theory, clause_length, prev_overlap_size, ignore_clauses
        # )
        max_overlap_indices, overlap_size = select_clauses(overlap_list)
        # max_overlap_indices, overlap_size = select_clauses_1(
        #     overlap_matrix, search_index, prev_overlap_size
        # )

        # print("Max overlapping indices = " + str(max_overlap_indices))
        # search_index = min(max_overlap_indices)

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
        # print(overlap_size, compression_size, str(operator_counter))
        if overlap_size - compression_size > 2:

            print(len(clause1))
            print_1d(clause1)
            print(len(clause2))
            print_1d(clause2)
            print("Overlap = " + str(len(clause1 & clause2)))
            clause_a, clause_b, clause_c = get_subclauses(clause1, clause2)
            print(
                "len(a) = "
                + str(len(clause_a))
                + ";\tlen(b) = "
                + str(len(clause_b))
                + ";\tlen(c) = "
                + str(len(clause_c))
            )
            print_2d(theory)
            print_2d(overlap_matrix)

        compression_counter += 1

        if len(compressed_clauses) == 2 and compression_size == 0:
            # Theory cannot be compressed any further
            # ignore_clauses.append((clause1, clause2))
            # overlap_matrix[max_overlap_indices[0]][max_overlap_indices[1]] = -1
            # continue
            # Initialize search index
            break
        elif is_lossless:
            # V - Operator is not applied
            # Continue compressing the theory
            # Delete the clauses used for last compression step
            theory, clause_length, overlap_list = delete_clause(
                theory, clause_length, overlap_list, max_overlap_indices
            )
            # theory, clause_length, overlap_matrix = delete_clause_1(
            #     theory, clause_length, overlap_matrix, max_overlap_indices
            # )

            # Insert clauses from compressed_clauses at appropriate lengths so that the theory remains sorted
            # theory |= compressed_clauses
            for clause in compressed_clauses:
                theory, clause_length, overlap_list, insert_index = insert_clause(
                    theory, clause_length, overlap_list, clause
                )
                # theory, clause_length, overlap_matrix, insert_index = insert_clause_1(
                #     theory, clause_length, overlap_matrix, clause
                # )
                # search_index = min(insert_index, search_index)

        else:
            # more_violations, p, n = count_violations(positives, negatives, theory)
            clause_validity = check_clause_validity(
                positives, negatives, clause1, clause2
            )
            # if (clause_validity and more_violations != initial_violations) or (
            #     not clause_validity and more_violations == initial_violations
            # ):
            #     print("Kuchh to gadbad hai daya")
            #     # Ignore clauses 1 and 2
            #     catch_it = 1

            # elif more_violations == initial_violations and clause_validity:
            if clause_validity:
                # Delete the clauses used for last compression step
                theory, clause_length, overlap_list = delete_clause(
                    theory, clause_length, overlap_list, max_overlap_indices
                )

                # Insert clauses from compressed_clauses at appropriate lengths so that the theory remains sorted
                # theory |= compressed_clauses
                for clause in compressed_clauses:
                    theory, clause_length, overlap_list, insert_index = insert_clause(
                        theory, clause_length, overlap_list, clause
                    )
                    # search_index = min(insert_index, search_index)

            # elif more_violations != initial_violations and not clause_validity:
            elif not clause_validity:
                # Perform a lossless step here, i.e., APply W-operator instead of V-operator
                (compressed_clauses, compression_size, is_lossless) = compress_pairwise(
                    clause1, clause2, lossless=True
                )

                # Delete the clauses used for last compression step
                theory, clause_length, overlap_list = delete_clause(
                    theory, clause_length, overlap_list, max_overlap_indices
                )

                # Insert clauses from compressed_clauses at appropriate lengths so that the theory remains sorted
                # theory |= compressed_clauses
                for clause in compressed_clauses:
                    theory, clause_length, overlap_list, insert_index = insert_clause(
                        theory, clause_length, overlap_list, clause
                    )
                    # theory, clause_length, overlap_matrix, insert_index = insert_clause_1(
                    #     theory, clause_length, overlap_matrix, clause
                    # )
                    # search_index = min(insert_index, search_index)

        # print_2d(theory)
        pbar.update(1)

    output_literal_length = get_literal_length(theory)
    output_bit_length = get_bit_length_for_theory(theory)

    if description_measure == "ll":
        output_length = output_literal_length
    elif description_measure == "se":
        output_length = output_bit_length

    print(
        "\nResultant Theory:\n\tLiteral Length = "
        + str(output_literal_length)
        + "\n\tBit Length = "
        + str(output_bit_length)
        + "\n\tNumber of clauses = "
        + str(len(theory))
        + "\n\t% Compressed Literals (For measure: "
        + str(description_measure)
        + ") = "
        + str(round(output_length * 100 / input_length, 2))
        + "%"
        + "\n\tOperator Count = "
        + str(operator_counter)
        + "\n\tTime Taken = "
        + str(time() - start_time)
        + " seconds"
        # + "\n\tOverlap Matrix = "
    )
    # print_2d(overlap_matrix)
    # print()
    # print_2d(theory)

    count = count_violations(positives, negatives, theory)

    return theory


def run_all(lossless=False):
    pass
    theory = mistle(*load_animal_taxonomy(), lossless=lossless)
    theory = mistle(*load_animal_taxonomy(switch_signs=True), lossless=lossless)

    # theory = mistle(*load_adult(), lossless=lossless)
    # theory = mistle(*load_adult(switch_signs=True), lossless=lossless)

    theory = mistle(*load_breast(), lossless=lossless)
    theory = mistle(*load_breast(switch_signs=True), lossless=lossless)

    # theory = mistle(*load_chess(), lossless=lossless)
    # theory = mistle(*load_chess(switch_signs=True), lossless=lossless)

    theory = mistle(*load_ionosphere(), lossless=lossless)
    theory = mistle(*load_ionosphere(switch_signs=True), lossless=lossless)

    # theory = mistle(*load_mushroom(), lossless=lossless)
    # theory = mistle(*load_mushroom(switch_signs=True), lossless=lossless)

    theory = mistle(*load_pima(), lossless=lossless)
    theory = mistle(*load_pima(switch_signs=True), lossless=lossless)

    theory = mistle(*load_tictactoe(), lossless=lossless)
    theory = mistle(*load_tictactoe(switch_signs=True), lossless=lossless)


row_pointer = 0
col_pointer = 0
new_var_counter = 1
invented_predicate_definition = {}
operator_counter = {"W": 0, "V": 0, "S": 0}

# positives, negatives = load_animal_taxonomy()
positives, negatives = load_adult()
theory = mistle(positives, negatives, lossless=False, description_measure="ll")
# theory = convert_to_theory(set(negatives))
# theory, clause_length = sort_theory(theory)
# count = count_violations(positives, negatives, theory)
# run_all(lossless=False)
