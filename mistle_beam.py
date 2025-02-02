from tqdm import tqdm
from copy import copy
from time import time
import numpy as np
from pycosat import solve
from collections import OrderedDict

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


def load_test1():
    # a = 1, b = (2, 3, 4), c = (5, 6)
    input_cnf = [(1, 2, 3, 4), (2, 3, 4, 5, 6)]
    output_cnf = [(-1, 5, 6), (2, 3, 4, 5, 6)]

    truth_assignments = []
    for a in [-1, 1]:
        for b1 in [-2, 2]:
            for b2 in [-3, 3]:
                for b3 in [-4, 4]:
                    for c1 in [-5, 5]:
                        for c2 in [-6, 6]:
                            truth_assignments.append((a, b1, b2, b3, c1, c2))

    pos2pos = []
    neg2neg = []
    pos2neg = []
    neg2pos = []

    for ass in truth_assignments:
        i = input_cnf + [(a,) for a in ass]
        o = output_cnf + [(a,) for a in ass]

        s_i = 0 if solve(i) == "UNSAT" else 1
        s_o = 0 if solve(o) == "UNSAT" else 1

        if s_i == 1 and s_o == 1:
            pos2pos.append(ass)
        elif s_i == 0 and s_o == 0:
            neg2neg.append(ass)
        elif s_i == 1 and s_o == 0:
            pos2neg.append(ass)
        elif s_i == 0 and s_o == 1:
            neg2pos.append(ass)

        print(ass, s_i, s_o)

    print("\nPos2Pos\t:", len(pos2pos))
    for p in pos2pos:
        print(p)

    print("\nNeg2Neg\t:", len(neg2neg))
    for n in neg2neg:
        print(n)

    print("\nPos2Neg\t:", len(pos2neg))
    for p in pos2neg:
        print(p)

    print("\nNeg2Pos\t:", len(neg2pos))
    for n in neg2pos:
        print(n)

    positives = [frozenset(ass) for ass in pos2pos]
    negatives = [frozenset([-1, -2, -3, -4]), frozenset([-2, -3, -4, -5, -6])]
    # positives = [frozenset(ass) for ass in pos2pos + pos2neg]
    # negatives = [frozenset(ass) for ass in neg2neg + neg2pos]

    print("\nPositives\t:", len(positives))
    for p in positives:
        print(p)

    print("\nNegatives\t:", len(negatives))
    for n in negatives:
        print(n)

    return positives, negatives


def load_test2():
    """
    Illustrative Toy Example used in the Paper
    :return:
    """

    positives = []
    positives.append(frozenset([1, 2]))
    positives.append(frozenset([2, 3]))
    positives.append(frozenset([2, 5]))

    negatives = []
    negatives.append(frozenset([-1, -2, -3, -4, -5]))
    negatives.append(frozenset([-1, -2, -3, -6, -7]))
    negatives.append(frozenset([-1, -3, -4]))
    negatives.append(frozenset([-2, -5, -6]))
    negatives.append(frozenset([-2, -3, -4]))
    negatives.append(frozenset([-2, -3, 4]))
    negatives.append(frozenset([-1, -3, -4]))
    negatives.append(frozenset([-2, 3]))

    return positives, negatives


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

    positives = [p1, p2, p3, p4]

    n1 = frozenset([-b, c, -d, -e, -f, g, -h, i, -j, k])
    n2 = frozenset([-b, -c, d, -e, -f, -g, h, i, -j, -l, m, n])
    n3 = frozenset([-b, -c, d, -e, -f, -g, h, i, -j, l, -m, n])
    n4 = frozenset([b, -c, -d, -e, -f, g, -h, -i, -j, k])
    n5 = frozenset([-b, -c, d, -e, -f, g, -h, i, -j])
    n6 = frozenset([-b, c, -d, -e, -f, g, -h, -i, -j, -k, -l, m])
    n7 = frozenset([-b, -c, -d, e, -f, g, -h, -i, -j, -k, l, -m])

    negatives = [n1, n2, n3, n4, n5, n6, n7]

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
    print_stats=False,
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
        partial_assignment = set()
        for i, j in enumerate(var_range):
            if str(j) in row[:-1]:
                partial_assignment.add(i + 1)
            elif negation:
                # Using closed world assumption to register absent variables as if they are false.
                partial_assignment.add(-(i + 1))

        if (not switch_signs and row[-1] == target_class[0]) or (
            switch_signs and row[-1] == target_class[1]
        ):
            negative_pas.append(frozenset(partial_assignment))
            neg_freq += 1
            if load_top_k and len(negative_pas) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not switch_signs and row[-1] == target_class[1]) or (
            switch_signs and row[-1] == target_class[0]
        ):
            positive_pas.append(frozenset(partial_assignment))
            pos_freq += 1
        else:
            print("Row found without target class at the end:\n")
            print(line)

        if load_tqdm:
            pbar.update(1)

    if print_stats:
        print_input_data_statistics(
            name,
            positive_pas,
            negative_pas,
            target_class,
            negation,
            load_top_k,
            switch_signs,
        )

    return (positive_pas, negative_pas)


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


def get_subclauses(clause1, clause2):
    clause_a = copy(set(clause1 - clause2))
    clause_b = copy(set(clause1 & clause2))
    clause_c = copy(set(clause2 - clause1))
    return clause_a, clause_b, clause_c


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


def check_pa_satisfiability(pa, clauses):
    return not (
        solve([tuple(clause) for clause in clauses] + [(a,) for a in pa]) == "UNSAT"
    )


def get_description_length(theory):
    dl = sum(theory.clause_length) + theory.theory_length - 1
    for misclassified_pa in theory.errors:
        dl += len(misclassified_pa)
    dl += len(theory.errors) - 1
    print("Description Length of theory\t:" + str(dl))
    return dl


class Mistle:
    def __init__(self, positives, negatives):
        self.positives = positives
        self.negatives = negatives

        self.total_positives = len(positives)
        self.total_negatives = len(negatives)

        self.theory = Theory(clauses=[], overlap_matrix=[], search_index=0)
        self.beam = None

    def get_literal_length(self, cnf):
        """
        :param cnf: a CNF/Theory
        :return: the total number of literals in the whole theory
        """
        return sum([len(clause) for clause in cnf])

    def check_clause_validity(self, input_clause1, input_clause2, output_clauses):
        """
        :param clauses: List of new clauses that can be added to the theory
        :return:
            True    if the new theory is consistent with the data
            False   otherwise
        """

        new_theory = copy(self.theory.clauses)
        new_theory.remove(input_clause1)
        new_theory.remove(input_clause2)
        new_theory += list(output_clauses)

        uncovered_positives = set()

        for pa in self.positives:
            if not check_pa_satisfiability(pa, new_theory):
                uncovered_positives.add(pa)

        return uncovered_positives

    def get_violations(self, positives, print_violations=True):
        """
        Get the number of positive partial assignments violated by the given theory.
        :param positives:
        :param print_violations:
        :return:
        """
        violated_pos = set()

        for pos in positives:
            if not check_pa_satisfiability(pos, self.theory.clauses):
                violated_pos.add(pos)

        if print_violations & len(violated_pos) > 0:
            print(
                len(violated_pos),
                " violations of positive partial assignments found for the learned theory.",
            )
        return violated_pos

    def learn(self, lossless=False, beam_size=1):
        """
        This function returns the minimal SAT theory that satisfies self.positives and does not satisfy self.negatives
        :param lossless: If lossless=True, then V-operator will not be considered for compression.
        :param beam_size: Not implemented yet (#TODO)
        :return: an instance of the class Theory
        """

        start_time = time()

        self.beam = Beam(beam_size)

        input_literal_length = self.get_literal_length(self.negatives)
        print(
            "\nInput Theory (with redundancies and inconsistencies):\n\tLiteral Length = "
            + str(input_literal_length)
            + "\n\tNumber of clauses = "
            + str(self.total_negatives)
        )

        # Remove redundant partial assignments
        self.positives = set(self.positives)
        self.negatives = set(self.negatives)

        self.theory.new_var_counter = (
            max([abs(l) for pa in self.positives | self.negatives for l in pa]) + 1
        )

        # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
        # TODO: Discuss with Clement on what to do with the inconsistent PAs
        inconsistent_pas = self.positives & self.negatives
        consistent_positives = self.positives - inconsistent_pas
        self.positives = copy(consistent_positives)
        self.negatives = self.negatives - inconsistent_pas

        print(
            "\nInput Theory (without redundancies and inconsistencies):\n\tLiteral Length = "
            + str(self.get_literal_length(self.negatives))
            + "\n\tNumber of clauses = "
            + str(len(self.negatives))
        )

        # Convert the set of -ve PAs to a theory
        self.theory.intialize(self.negatives)
        self.theory.errors = self.get_violations(self.positives)

        self.total_positives = len(self.positives)
        self.total_negatives = len(self.negatives)
        self.theory.theory_length = self.total_negatives

        overlap_size = None
        compression_counter = 0

        pbar = tqdm(total=int(1.1 * self.theory.theory_length + 100))
        pbar.set_description("Compressing Clauses")
        while True:
            # TODO: Implement Beam Search
            new_beam = Beam(beam_size)

            while self.beam:
                current_theory = self.beam.pop()
                next_beam = Beam(beam_size)

            prev_overlap_size = overlap_size
            max_overlap_indices, overlap_size = self.theory.select_clauses(
                prev_overlap_size
            )

            if overlap_size < 2:
                print("Cannot compress anymore: Max overlap size < 2.")
                break

            clause1 = self.theory.clauses[max_overlap_indices[0]]
            clause2 = self.theory.clauses[max_overlap_indices[1]]

            (
                compressed_clauses,
                compression_size,
                applied_operator,
            ) = self.theory.compress_pairwise(clause1, clause2, lossless=lossless)
            # assert overlap_size - 2 <= compression_size

            compression_counter += 1

            if applied_operator is None:
                assert len(compressed_clauses) == 2 and compression_size == 0
                # Theory cannot be compressed any further
                # Initialize search index
                print("Cannot compress the theory any further.")
                break
            elif applied_operator in ["R", "S", "W"]:
                # V - Operator is not applied
                # Continue compressing the theory

                # Delete the clauses used for last compression step
                self.theory.delete_clauses(max_overlap_indices)

                # Insert clauses from compressed_clauses at appropriate lengths so that the theory remains sorted
                for clause in compressed_clauses:
                    self.theory.insert_clause(clause)

                self.theory.compression += compression_size

            elif applied_operator == "V":
                uncovered_positives = self.check_clause_validity(
                    clause1, clause2, compressed_clauses
                )

                if (
                    self.get_literal_length(uncovered_positives)
                    + len(uncovered_positives)
                    - 1
                    > compression_size
                ):
                    # if (
                    #     len(uncovered_positives) > compression_size
                    # ):  # Comment this line for ignoring errors
                    # if len(uncovered_positives) > 0:# Uncomment this line for ignoring errors
                    # Perform a lossless step here, i.e., Apply W-operator instead of V-operator
                    self.theory.operator_counter["V"] -= 1

                    (
                        compressed_clauses2,
                        compression_size,
                        applied_operator,
                    ) = self.theory.compress_pairwise(
                        clause1, clause2, lossless=True, force_w=True
                    )

                    assert applied_operator == "W"

                    self.theory.delete_clauses(max_overlap_indices)
                    self.theory.compression += compression_size

                    for clause in compressed_clauses:
                        self.theory.insert_clause(clause)

                else:
                    assert not self.theory.errors.intersection(uncovered_positives)
                    self.theory.errors |= uncovered_positives
                    self.positives -= (
                        uncovered_positives  # Comment this line for ignoring errors
                    )
                    self.theory.delete_clauses(max_overlap_indices)
                    self.theory.compression += compression_size

                    for clause in compressed_clauses:
                        self.theory.insert_clause(clause)

            pbar.update(1)

        pbar.close()
        output_literal_length = sum(self.theory.clause_length)
        compression = round((1 - output_literal_length / input_literal_length) * 100, 2)

        print(
            "\nResultant Theory:\n\tLiteral Length = "
            + str(output_literal_length)
            + "\n\t# of clauses = "
            + str(self.theory.theory_length)
            + "\n\t% Compression = "
            + str(compression)
            + "%"
            + "\n\tOperator Count = "
            + str(self.theory.operator_counter)
            + "\n\tTime Taken = "
            + str(time() - start_time)
            + " seconds"
        )

        p = self.get_violations(consistent_positives, print_violations=False)

        if self.theory.errors != p:
            print("# Violations\t: " + str(len(p)))
            print("# Errors\t\t: " + str(len(self.theory.errors)))
            pass

        # assert self.errors == p

        return self.theory, compression


class Theory:
    def __init__(self, clauses, overlap_matrix, search_index):
        self.clauses = clauses
        self.overlap_matrix = overlap_matrix
        self.search_index = search_index
        self.clause_length = []  # List of lengths of all the clauses of this theory
        self.compression = 0
        self.theory_length = len(clauses)  # Total number of clauses in the theory

        self.update_dict = {}
        self.update_threshold = (
            1  # The number of new predicates to invent before updating the theory
        )
        self.errors = set()

        self.operator_counter = {"W": 0, "V": 0, "S": 0, "R": 0}
        self.invented_predicate_definition = OrderedDict()
        self.new_var_counter = None

    def intialize(self, partial_assignments):
        # Construct a theory from the partial assignments
        for pa in partial_assignments:
            self.clauses.append(frozenset([-literal for literal in pa]))
            self.clause_length.append(len(pa))

        # Sort the theory
        self.theory_length = len(self.clauses)
        index = list(range(self.theory_length))
        index.sort(key=self.clause_length.__getitem__, reverse=True)
        self.clause_length = [self.clause_length[i] for i in index]

        self.clauses = [self.clauses[i] for i in index]

        # Initialize the overlap matrix
        for i, clause1 in enumerate(self.clauses):
            self.overlap_matrix.append([0] * self.theory_length)
            for j, clause2 in enumerate(self.clauses[i + 1 :]):
                self.overlap_matrix[i][i + j + 1] = len(clause1 & clause2)

    def compress_pairwise(self, clause1, clause2, lossless=False, force_w=False):
        """
        Compress clause1 and clause2
        :param clause1:
        :param clause2:
        :param lossless: Apply V-operator only if lossless=False
        :return:
            A list of the new clauses obtained after compression
            The decrease in literal length achieved during compression
            A character variable specifying the type of operator used. It is None if no operator is applied.
        """

        clause_a, clause_b, clause_c = get_subclauses(clause1, clause2)
        resolution_applicable = False

        if (
            len(clause_a) == 1
            and len(clause_c) == 1
            and copy(clause_a).pop() == -copy(clause_c).pop()
        ):
            resolution_applicable = True

        if len(clause_a) == 0:
            # Apply subsumption operator on (b1; b2; b3), (b1; b2; b3; c1; c2; c3)
            self.operator_counter["S"] += 1
            # return {clause1}, len(clause_b), "S"
            return {clause1}, len(clause_b) + 1, "S"

        elif len(clause_c) == 0:
            # Apply subsumption operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3)
            self.operator_counter["S"] += 1
            # return {clause2}, len(clause_b), "S"
            return {clause2}, len(clause_b) + 1, "S"

        elif resolution_applicable:
            # Apply complementation operator on (a; b1; b2; b3), (b1; b2; b3; -a)
            # Return (b1; b2; b3)
            self.operator_counter["R"] += 1
            # return {frozenset(clause_b)}, len(clause_b) + 2, "R"
            return {frozenset(clause_b)}, len(clause_b) + 3, "R"

        elif len(clause_b) == 0:
            # Cannot compress (a1; a2; a3), (c1; c2; c3)
            print_1d(clause1)
            print_1d(clause2)
            print("No Overlap Found")
            return {clause1, clause2}, 0, None

        elif not lossless and len(clause_a) == 1 and len(clause_b) > 1:
            # Apply V-operator on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
            # Return (a; b1; b2; b3), (c1; c2; c3; -a)
            a = clause_a.pop()
            clause_c.add(-a)
            self.operator_counter["V"] += 1
            return {clause1, frozenset(clause_c)}, len(clause_b) - 1, "V"

        elif not lossless and len(clause_c) == 1 and len(clause_b) > 1:
            # Apply V-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
            # Return (a1; a2; a3; -c), (b1; b2; b3; c)
            c = clause_c.pop()
            clause_a.add(-c)
            self.operator_counter["V"] += 1
            return {frozenset(clause_a), clause2}, len(clause_b) - 1, "V"

        # elif not force_w and len(clause_b) < 3:
        elif not force_w and len(clause_b) < 4:
            # print("Application of W-Operator is infeasible due to overlap of only " + str(len(clause_b)) + "
            # literals.")
            return {clause1, clause2}, 0, None

        else:
            # This is the case where len(clause_a) > 1, len(clause_b) > 2, and len(clause_c) > 1.
            # Apply W-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
            # Return (a1; a2; a3; -z), (b1; b2; b3; z), (c1; c2; c3, -z)

            # Invent a new predicate if the definition of a predicate is new.
            if clause_b not in self.invented_predicate_definition.values():
                new_var = self.get_new_var()
                clause_a.add(-new_var)
                self.invented_predicate_definition[new_var] = copy(clause_b)
                clause_b.add(new_var)
                clause_c.add(-new_var)

                # Update other clauses in the theory that contain 'clause_b'
                self.update_clause(new_var, clause_b)

            else:
                # self.w = True
                for var, clause in self.invented_predicate_definition.items():
                    if clause == clause_b:
                        clause_a.add(-var)
                        clause_b.add(var)
                        clause_c.add(-var)

            self.operator_counter["W"] += 1

            # return (
            #     {frozenset(clause_a), frozenset(clause_b), frozenset(clause_c)},
            #     len(clause_b) - 3,
            #     "W",
            # )
            # TODO: Think about the feasibility for the case when W-operator is supposed to applied for more than 2 clauses
            return (
                {frozenset(clause_a), frozenset(clause_b), frozenset(clause_c)},
                len(clause_b) - 4,
                "W",
            )

    def select_clauses(self, prev_overlap_size):
        """
        Select clauses for next compression step WITH a compression matrix
        :param prev_overlap_size: Previous Overlap Size
        :return:
            max_overlap_indices: The indices of 2 clauses that have the maximum overlap size
            max_overlap_size: The maximum overlap size
        """

        max_overlap_size = 0
        max_overlap_indices = (0, 0)
        # Only search the overlap matrix (After self.search_index)
        for i, row in enumerate(self.overlap_matrix[self.search_index :]):
            for j, overlap in enumerate(row[self.search_index + i + 1 :]):
                if overlap > max_overlap_size:
                    max_overlap_size = overlap
                    max_overlap_indices = (
                        self.search_index + i,
                        self.search_index + i + 1 + j,
                    )
                    if prev_overlap_size and max_overlap_size >= prev_overlap_size:
                        break

        if prev_overlap_size and max_overlap_size < prev_overlap_size:
            # Search the initial part of the matrix too (Before self.search_index)

            max_overlap_size1 = 0
            max_overlap_indices1 = (0, 0)
            for i, row in enumerate(self.overlap_matrix[: self.search_index]):
                for j, overlap in enumerate(row[i + 1 : self.search_index]):
                    if overlap > max_overlap_size1:
                        max_overlap_size1 = overlap
                        max_overlap_indices1 = (i, i + 1 + j)

            if max_overlap_size1 >= max_overlap_size:
                max_overlap_indices = max_overlap_indices1
                max_overlap_size = max_overlap_size1

        self.search_index = max_overlap_indices[0]

        return max_overlap_indices, max_overlap_size

    def delete_clauses(self, indices):

        del self.clause_length[indices[1]]
        del self.clause_length[indices[0]]

        del self.overlap_matrix[indices[1]]
        del self.overlap_matrix[indices[0]]
        for i in range(self.theory_length - 2):
            del self.overlap_matrix[i][indices[1]]
            del self.overlap_matrix[i][indices[0]]

        del self.clauses[indices[1]]
        del self.clauses[indices[0]]

        self.theory_length -= 2

    def insert_clause(self, clause):

        i = int(self.theory_length / 2)
        start = 0
        end = self.theory_length
        index_found = False
        index = None
        l = len(clause)
        while i < self.theory_length:
            if l <= self.clause_length[-1]:
                # Insert element at the end of the list
                index = self.theory_length
                index_found = True
                break
            elif l >= self.clause_length[0]:
                # Insert element around the beginning at the place when the length drops right after
                for j, cl in enumerate(self.clause_length):
                    if cl < l:
                        index = j
                        break
                index_found = True
                break
            elif self.clause_length[i] == l:
                for j, cl in enumerate(self.clause_length[i + 1 :]):
                    if cl < l:
                        index = j + i + 1
                        break
                index_found = True
                break
            elif i + 1 < len(self.clause_length) and self.clause_length[i + 1] == l:
                for j, cl in enumerate(self.clause_length[i + 2 :]):
                    if cl < l:
                        index = j + i + 2
                        break
                index_found = True
                break
            elif (
                i + 1 < len(self.clause_length)
                and self.clause_length[i] > l > self.clause_length[i + 1]
            ):
                # Insert element at/before (i + 1) if
                # clause_length[i] >= len(clause) >= clause_length[i + 1]
                index = i + 1
                index_found = True
                break
            elif l > self.clause_length[i]:
                end = i
                i = int((i + start) / 2)
            elif l < self.clause_length[i]:
                start = i
                i = int((end + i) / 2)
            else:
                i += 1

        if index_found is False:
            # This is the case where a clause is being inserted to an empty theory
            index = 0

        new_overlap_row = [0] * (self.theory_length + 1)
        self.overlap_matrix[index:index] = [new_overlap_row]

        self.clauses[index:index] = [clause]

        for j, clause2 in enumerate(self.clauses):
            if j == index:
                continue

            overlap = len(clause & clause2)

            if j < index:
                self.overlap_matrix[j][index:index] = [overlap]
                self.overlap_matrix[index][j] = 0
            elif j > index:
                self.overlap_matrix[j][index:index] = [0]
                self.overlap_matrix[index][j] = overlap

        self.clause_length[index:index] = [len(clause)]

        if index < self.search_index:
            self.search_index = index
        # else:
        #     # print(np.array(self.clause_length))
        #     prev_l = self.clause_length[0]
        #     for l in self.clause_length[1:]:
        #         if l < prev_l:
        #             prev_l = l
        #         elif l > prev_l:
        #             print(np.array(self.clause_length))
        #             break

        # self.search_index = min(self.search_index, index)
        self.theory_length += 1

    def update_clause(self, new_var, clause):
        """
        Update the theory to replace every occurrence of 'clause' by 'new_var'
        :param new_var:
        :param clause:
        :return:
        """
        self.update_dict[new_var] = clause

        if len(self.update_dict) == self.update_threshold:
            clause_list = []
            new_var_list = []
            clause_lengths = []
            for k, v in self.update_dict.items():
                new_var_list.append(k)
                clause_list.append(v)
                clause_lengths.append(len(v))

            index = list(range(self.update_threshold))
            index.sort(key=clause_lengths.__getitem__, reverse=True)

            clause_list = [clause_list[i] for i in index]
            new_var_list = [new_var_list[i] for i in index]

            delete_indices = []
            updated_clauses = []
            for i, clause in enumerate(self.clauses):

                for new_var, clause_def in zip(new_var_list, clause_list):
                    if clause_def.issubset(clause):
                        new_clause = set(clause) - clause_def
                        new_clause.add(-new_var)

                        updated_clauses.append(new_clause)
                        delete_indices.append(i)
                        break

            if len(delete_indices) > 0:
                print("Other clause found in theory that has the subset.")

            # Delete the old clauses from the theory
            delete_indices = delete_indices[::-1]
            for index in delete_indices:
                del self.clause_length[index]

                del self.overlap_matrix[index]
                for i in range(self.theory_length - 1):
                    del self.overlap_matrix[i][index]

                del self.clauses[index]

                self.theory_length -= 1

            # Insert the updated clauses in the theory
            for clause in updated_clauses:
                self.insert_clause(clause)

            self.update_dict = {}

    def get_new_var(self):
        """
        Invent a new predicate
        :return: An integer (updated counter) which has not been used as a literal before
        """
        new_var = self.new_var_counter
        self.new_var_counter += 1
        return new_var

    def __len__(self):
        return self.theory_length


class Beam:
    def __init__(self, size):
        self._size = size
        self._candidates = []  # A list of candidate theories to be stored in the beam

    def _bottom_score(self):
        if self._candidates:
            return self._candidates[-1].compression
        else:
            return 0

    def _insert(self, candidate):
        for i, x in enumerate(self._candidates):
            if x.is_equivalent(candidate):
                raise ValueError("duplicate")
            elif x.compression < candidate.compression:
                self._candidates.insert(i, candidate)
                return False
        self._candidates.append(candidate)
        return True

    def push(self, candidate):
        """Adds a candidate to the beam.

        :param candidate: candidate to add
        :return: True if candidate was accepted, False otherwise
        """
        if (
            len(self._candidates) < self._size
            or candidate.compression > self._bottom_score()
        ):
            #  We should add it to the beam.
            try:
                is_last = self._insert(candidate)
                if len(self._candidates) > self._size:
                    self._candidates.pop(-1)
                    return not is_last
            except ValueError:
                return False
            return True
        return False

    def pop(self):
        return self._candidates.pop(0)

    def __bool__(self):
        return bool(self._candidates)

    def __nonzero__(self):
        return bool(self._candidates)

    def __str__(self):
        s = "==================================\n"
        for candidate in self._candidates:
            s += str(candidate.compression) + "\n"
        s += "=================================="
        return s


if __name__ == "__main__":
    positives, negatives = load_ionosphere()
    start_time = time()
    mistle = Mistle(positives, negatives)
    theory, compression1 = mistle.learn(beam_size=2)
    print("Total time\t: " + str(time() - start_time) + " seconds.")
    initial_dl = -1
    for pa in positives + negatives:
        initial_dl += 1
        initial_dl += len(pa)

    final_dl = get_description_length(theory)
    compression2 = (initial_dl - final_dl) / float(initial_dl)
    print("Compression (wrt Literal Length)\t: " + str(compression1))
    print("Initial DL\t: " + str(initial_dl))
    print("Final DL\t: " + str(final_dl))
    print("Compression (wrt Description Length)\t: " + str(compression2))
