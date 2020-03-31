from tqdm import tqdm
from copy import copy
from pycosat import solve
from time import time
import math
import sys
from collections import Counter
from pattern_mining import compute_itemsets


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


def load_test():
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
    raw_data=False,
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
        if raw_data:
            partial_assignment = [int(literal) for literal in row[:-1]]
        else:
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


def check_pa_satisfiability(pa, clauses):
    if clauses is None or len(clauses) == 0:
        return None
    else:
        return not (
            solve([tuple(clause) for clause in clauses] + [(a,) for a in pa]) == "UNSAT"
        )


def get_literal_length(clauses):
    """
    :param clauses: a CNF/Theory
    :return: the total number of literals in the whole theory
    """
    return sum([len(clause) for clause in clauses])


def get_entropy(clauses):
    """
    :param clauses: a list of clauses
    :return: the total number of bits required to represent the input theory
    """

    counts = Counter()
    total_literals = 0
    for clause in clauses:
        for literal in clause:
            counts[literal] += 1
            total_literals += 1

    entropy = 0
    for v in counts.values():
        p = float(v) / total_literals
        if p > 0.0:
            entropy -= math.log(p, 2)

    return entropy


def log_star(input):
    # From the footnote on page 5 of the paper https://hal.inria.fr/hal-02505913/document
    result = math.log(2.865064)
    n = copy(input)
    while True:
        n = math.log(n)
        if n <= 0:
            break
        else:
            result += n

    # print("Log*N( " + str(input) + ")\t= " + str(result))
    return result


def get_c_entropy(clauses, alphabet_size):
    """
    :param clauses: a list of clauses
    :return: the total number of bits required to represent the input theory
    """
    c_entropy = 0
    if len(clauses) == 0:
        return c_entropy

    for j, clause in enumerate(clauses):

        for i, literal in enumerate(clause):
            c_entropy -= math.log(1.0 / (2 * len(clause) + 1 - 2 * i), 2)

        # c_entropy -= math.log(1.0 / (2 * len(clauses) + 1 - 2 * len(clause)), 2)
        c_entropy -= math.log(1.0 / (2 * len(clauses) + 1 - 2 * j), 2)

    c_entropy += log_star(2 * alphabet_size)
    c_entropy += log_star(len(clauses))

    return c_entropy


def get_dl(dl_measure, clauses, errors, alphabet_size):
    if dl_measure == "ll":  # Literal Length
        return get_literal_length(clauses)
    elif dl_measure == "sl":  # Symbol Length
        return (
            get_literal_length(clauses + list(errors)) + len(clauses + list(errors)) - 1
        )
    elif dl_measure == "se":  # Shanon Entropy
        return get_entropy(clauses) + get_entropy(errors)
    elif dl_measure == "ce":  # Clement Entropy
        return get_c_entropy(clauses, alphabet_size) + get_c_entropy(
            errors, alphabet_size
        )


class Eclat:
    def __init__(self, minsup):
        self.minsup = minsup
        # self.freq_items = []
        self.freq_items = {}
        self.pruned_itemsets = set()

    def read_data(self, input):
        data = {}
        for i, pa in enumerate(input):
            for item in pa:
                if item not in data:
                    data[item] = set()
                data[item].add(i)
        return data

    def eclat(self, prefix, items, dict_id):
        while items:
            i, itids = items.pop()
            if len(itids) >= self.minsup:
                # if len(prefix) > 0:  # Only store patterns with length > 1
                # self.freq_items.append((frozenset(prefix + [i]), itids))

                if frozenset(prefix) in self.freq_items and len(
                    self.freq_items[frozenset(prefix)]
                ) <= len(itids):
                    self.pruned_itemsets.add(frozenset(prefix))
                    del self.freq_items[frozenset(prefix)]
                # elif frozenset(prefix + [i]) not in self.pruned_itemsets:
                if frozenset(prefix + [i]) in self.pruned_itemsets:
                    old_itids = self.freq_items[frozenset(prefix + [i])]
                    print("Old len(itids)\t: " + str(len(old_itids)))
                    print("New len(itids)\t: " + str(len(itids)))
                    print("Intersection len(itids)\t: " + str(itids & old_itids))

                self.freq_items[frozenset(prefix + [i])] = itids

                suffix = []
                for j, ojtids in items:
                    jtids = itids & ojtids
                    if len(jtids) >= self.minsup:
                        suffix.append((j, jtids))

                dict_id += 1
                self.eclat(
                    prefix + [i],
                    sorted(suffix, key=lambda item: len(item[1]), reverse=True),
                    dict_id,
                )

    def get_Frequent_Itemsets(self, input):

        start_time = time()
        prefix = []
        dict_id = 0

        data = self.read_data(input)
        sorted_data = sorted(data.items(), key=lambda item: len(item[1]), reverse=True)
        self.eclat(prefix, sorted_data, dict_id)
        # self.freq_items = list(self.freq_items.items())

        # print("Frequent Itemsets\t:")
        # for item, clause_ids in self.freq_items.items():
        #     print(" {}\t: {}".format(list(item), clause_ids))
        # print()

        # sort freq items descending wrt change in DL (wrt W-op), length of the itemset, frequency

        # self.freq_items.sort(
        #     key=lambda item: (
        #         len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
        #         len(item[0]),
        #         len(item[1]),
        #     ),
        #     reverse=True,
        # )

        print(
            str(len(self.freq_items))
            + " Frequent Itemsets are mined in "
            + str(time() - start_time)
            + " seconds."
        )
        # for item, clause_ids in self.freq_items:
        #     print(" {}\t: {}".format(list(item), clause_ids))
        # print()

        return self.freq_items


class Mistle:
    def __init__(self, positives, negatives):
        self.positives = positives
        self.negatives = negatives

        self.total_positives = len(positives)
        self.total_negatives = len(negatives)

        self.theory = Theory(clauses=[], overlap_matrix=[], search_index=0)
        self.beam = None
        self.initial_dl = None

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

    def learn(self, minsup, dl_measure):

        # This line of code assumes that variable numbers start from 1:
        alphabet_size = max(
            [abs(literal) for pa in self.positives + self.negatives for literal in pa]
        )
        self.initial_dl = get_dl(
            dl_measure, self.positives + self.negatives, [], alphabet_size
        )

        # Remove redundant partial assignments
        self.positives = set(self.positives)
        self.negatives = set(self.negatives)

        self.theory.new_var_counter = alphabet_size + 1

        # Remove inconsistent partial assignments (Those pas that are both classified as +ves and -ves) in the data
        inconsistent_pas = self.positives & self.negatives
        consistent_positives = self.positives - inconsistent_pas
        self.positives = copy(consistent_positives)
        self.negatives = self.negatives - inconsistent_pas

        # Convert the set of -ve PAs to a theory
        success = self.theory.intialize(
            self.positives, self.negatives, minsup, dl_measure
        )
        if not success:
            # Return empty theory
            return None, 0

        self.total_positives = len(self.positives)
        self.total_negatives = len(self.negatives)
        self.theory.theory_length = self.total_negatives

        while True:
            success = self.theory.compress_theory()
            if success == "ignore_itemset":
                del self.theory.freq_items[0]
            elif not success:
                break

            # print(
            #     "--------------------------Frequent Itemsets--------------------------\t:"
            # )
            # for item, clause_ids in self.theory.freq_items:
            #     print(" {}\t: {}".format(list(item), clause_ids))
            # print("\n")

        final_dl = get_dl(
            dl_measure,
            self.theory.clauses,
            self.theory.errors,
            self.theory.new_var_counter - 1,
        )
        compression = (self.initial_dl - final_dl) / float(self.initial_dl)
        print("Initial DL\t\t\t\t: " + str(self.initial_dl))
        print("Final DL\t\t\t\t: " + str(final_dl))
        print(
            "Compression (wrt "
            + str(self.theory.dl_measure)
            + ")\t: "
            + str(compression)
        )
        print("Operator Counters\t\t: " + str(self.theory.operator_counter))

        return self.theory, compression


class Theory:
    def __init__(self, clauses, overlap_matrix, search_index):
        self.clauses = clauses
        self.overlap_matrix = overlap_matrix
        self.search_index = search_index
        self.clause_length = []  # List of lengths of all the clauses of this theory
        self.theory_length = len(clauses)  # Total number of clauses in the theory
        self.invented_predicate_definition = dict()
        self.freq_items = []
        self.errors = set()
        self.positives = set()
        self.minsup = None
        self.dl = None
        self.dl_measure = None
        self.operator_counter = {"W": 0, "V": 0, "S": 0, "R": 0, "T": 0}
        self.new_var_counter = None

    def intialize(self, positives, negatives, minsup, dl_measure):

        if (
            len(negatives) < 1
        ):  # We still allow a theory to be learned and compressed if there are some negatives and no positives.
            return False

        # Construct a theory from the partial assignments
        for pa in negatives:
            self.clauses.append(frozenset([-literal for literal in pa]))
            self.clause_length.append(len(pa))

        self.minsup = minsup

        start_time1 = time()
        freq_items1 = compute_itemsets(self.clauses, minsup / len(self.clauses), "LCM")
        total_time1 = time() - start_time1
        print("Length of freq items 1\t: " + str(len(freq_items1)))
        print("Time of freq items 1\t: " + str(total_time1))

        start_time2 = time()
        eclat = Eclat(minsup=minsup)
        freq_items2 = eclat.get_Frequent_Itemsets(self.clauses)
        total_time2 = time() - start_time2
        print("Length of freq items 2\t: " + str(len(freq_items2)))
        print("Time of freq items 2\t: " + str(total_time2))

        assert len(freq_items2) >= len(freq_items1)

        if len(freq_items2) == len(freq_items1):
            self.freq_items = list(freq_items2.items())
        else:
            for i, (itemset, frequency) in enumerate(freq_items1):
                item = frozenset(itemset)
                if item not in freq_items2:
                    print("Itemset not found\t: " + str(item))
                else:
                    self.freq_items.append((item, freq_items2[item]))

        self.freq_items.sort(
            key=lambda item: (
                len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
                len(item[0]),
                len(item[1]),
            ),
            reverse=True,
        )

        self.positives = positives
        self.errors = self.get_violations(positives)
        self.dl = get_dl(
            dl_measure, list(positives | negatives), [], self.new_var_counter - 1
        )
        print("DL of initial theory\t: " + str(self.dl))
        self.dl_measure = dl_measure
        return True

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

    def get_violations(self, positives, print_violations=True):
        """
        Get the number of positive partial assignments violated by the given theory.
        :param positives:
        :param print_violations:
        :return:
        """
        violated_pos = set()

        for pos in positives:
            if not check_pa_satisfiability(pos, self.clauses):
                violated_pos.add(pos)

        if print_violations & len(violated_pos) > 0:
            print(
                len(violated_pos),
                " violations of positive partial assignments found for the learned theory.",
            )
        return violated_pos

    def get_compression(self, input_clauses, op, output_clauses):

        new_theory = copy(self.clauses)
        for clause in input_clauses:
            new_theory.remove(clause)
        new_theory += list(output_clauses)

        uncovered_positives = set()
        if op in {"V", "T"}:
            for pa in self.positives:
                if not check_pa_satisfiability(pa, new_theory):
                    uncovered_positives.add(pa)

        return (
            uncovered_positives,
            get_dl(
                self.dl_measure,
                new_theory,
                list(self.errors | uncovered_positives),
                self.new_var_counter - 1,
            ),
        )

    def apply_best_operator(self, input_clause_list, possible_operations):
        success = "ignore_itemset"
        operator_precedence = ["S", "R", "W", "V", "T", None]
        min_dl = self.dl
        best_operator = None
        new_errors = None
        output_clause_list = None
        possible_entropies = []
        for op, output_clauses in possible_operations:
            # errors, compression = self.get_compression(
            #     input_clause_list, op, output_clauses
            # )
            # if compression > max_compression or (
            #     compression == max_compression
            #     and operator_precedence.index(op)
            #     < operator_precedence.index(best_operator)
            # ):

            errors, dl = self.get_compression(input_clause_list, op, output_clauses)
            possible_entropies.append((op, dl))
            if dl < min_dl or (
                dl == min_dl
                and operator_precedence.index(op)
                < operator_precedence.index(best_operator)
            ):
                min_dl = dl
                best_operator = op
                new_errors = errors
                output_clause_list = list(output_clauses)
                success = True

        if min_dl == self.dl:
            print(
                "Ignoring itemset\t: Initial DL = "
                + str(self.dl)
                + "\t; Possible DLs\t: "
                + str(possible_entropies)
            )
            success = "ignore_itemset"

        if success is True:
            # Update old_theory
            self.operator_counter[best_operator] += 1
            if best_operator == "W":
                new_var = self.get_new_var()
                for i, clause in enumerate(output_clause_list):
                    if "#" in clause:
                        output_clause_list[i].remove("#")
                        output_clause_list[i].add(new_var)
                    else:
                        output_clause_list[i].remove("-#")
                        output_clause_list[i].add(-new_var)
                    output_clause_list[i] = frozenset(output_clause_list[i])
                subclause = self.invented_predicate_definition["#"]
                assert new_var not in self.invented_predicate_definition
                self.invented_predicate_definition[new_var] = subclause

            assert "#" in self.invented_predicate_definition
            del self.invented_predicate_definition["#"]

            self.errors |= new_errors
            self.positives -= new_errors
            # self.total_compression += max_compression
            self.theory_length += len(output_clause_list) - len(input_clause_list)

            pruned_indices = list(self.freq_items[0][1])

            # Delete indices that are in pruned indices and decrease indices of other clauses correspondingly
            buffer = sorted(pruned_indices)
            # print("Initial Buffer\t: " + str(buffer))
            decrement = 0
            replace_dict = {}
            for i in range(len(self.clauses)):
                if i < buffer[0]:
                    replace_dict[i] = i - decrement
                elif i == buffer[0]:
                    del buffer[0]
                    decrement += 1
                    if len(buffer) == 0:
                        break

            # print("Final Buffer\t: " + str(buffer))
            # print("Replace Dict\t: " + str(replace_dict))

            # Replace the indices of clauses wrt the replacement dictionary
            increment = len(replace_dict)
            # new_freq_items = []
            del self.freq_items[0]
            prune_itemsets = []
            for i, (itemset, clause_ids) in enumerate(self.freq_items):

                new_clause_ids = set()
                # Repalce ids of old existing clauses
                for clause_id in clause_ids:
                    if clause_id in replace_dict:
                        new_clause_ids.add(replace_dict[clause_id])

                # Add ids of new clause covering this itemset
                for j, clause in enumerate(output_clause_list):
                    if itemset.issubset(clause):
                        new_clause_ids.add(j + increment)

                if len(new_clause_ids) >= 2:
                    self.freq_items[i] = (itemset, new_clause_ids)
                else:
                    prune_itemsets.append(i)

            prune_itemsets.sort(reverse=True)
            for i in prune_itemsets:
                del self.freq_items[i]

            self.freq_items.sort(
                key=lambda item: (
                    len(item[0]) * len(item[1]) - (len(item[0]) + len(item[1]) + 1),
                    len(item[0]),
                    len(item[1]),
                ),
                reverse=True,
            )

            print(
                str(len(self.freq_items))
                + "\tfrequent itemsets left after applying "
                + best_operator
                + " operator with output \t: "
                + str(output_clause_list)
            )
            # print("New Frequent Itemsets\t:")
            # for item, clause_ids in self.freq_items:
            #     print(" {}\t: {}".format(list(item), len(clause_ids)))
            # print()

            pruned_indices.sort(reverse=True)
            for i in pruned_indices:
                del self.clauses[i]
                del self.clause_length[i]

            for clause in output_clause_list:
                self.clauses.append(clause)
                self.clause_length.append(len(clause))

            # assert min_entropy == get_entropy(self.clauses) + get_entropy(self.errors)
            # self.entropy = min_entropy
            self.dl = min_dl
        return success

    def compress_theory(self):
        # TODO: Make it more efficient once it is complete. Reduce the number of iterations on old_clause_list/residue
        if len(self.freq_items) == 0:
            return False
        subclause, clause_ids = self.freq_items[0]
        if len(clause_ids) == 1:
            return "ignore_itemset"

        old_clause_list = []  # A list of input clauses (to be compressed)
        residue = []  # A list of input clauses without the shared subclause
        possible_operations = []

        for clause_id in clause_ids:
            clause = self.clauses[clause_id]
            old_clause_list.append(clause)
            residue.append(clause - subclause)

        # Check if S-operator is applicable
        s_applicable = False
        for clause in old_clause_list:
            if clause == subclause:
                # S-operator is applicable
                s_applicable = True
                possible_operations.append(("S", [subclause]))
                # self.operator_counter["S"] += 1
                # return True

        # Check if R-operator is applicable
        r_applicable = False
        r_residue = [tuple(clause) for clause in residue]
        if solve(r_residue) == "UNSAT":
            # R-operator is applicable (R-operator is a special case when applying T-operator is lossless.
            # Example: If input is [(a,b), (a,c), (a,-b,-c)], then the output is [(a)].
            # Here, then input is logically equivalent to the output (Lossless Truncation).
            r_applicable = True
            possible_operations.append(("R", [subclause]))
            # return True

        if not s_applicable and not r_applicable:
            possible_operations.append(("T", [subclause]))

        # Check if V-operator is applicable
        v_literals = []
        for clause in residue:
            if len(clause) == 1:
                v_literals.append(next(iter(clause)))
                break

        if v_literals:
            for v_literal in v_literals:
                v_output = []
                for clause in residue:
                    if len(clause) == 1 and v_literal == next(iter(clause)):
                        new_clause = set(subclause)
                        new_clause.add(v_literal)
                        v_output.append(frozenset(new_clause))
                    else:
                        new_clause = set(clause)
                        new_clause.add(-v_literal)
                        v_output.append(frozenset(new_clause))
                possible_operations.append(("V", v_output))

        # Consider W-operator

        # Use '#' as a special newly invented variable.
        # If/once W-operator is accepted for compression, it will be replaced by a self.new_var_counter()
        new_clause = set(subclause)
        new_clause.add("#")
        w_output = [new_clause]
        self.invented_predicate_definition["#"] = subclause

        for clause in residue:
            new_clause = set(clause)
            new_clause.add("-#")
            w_output.append(new_clause)

        possible_operations.append(("W", w_output))

        return self.apply_best_operator(old_clause_list, possible_operations)


if __name__ == "__main__":
    filename = sys.argv[1]
    nb_rows = int(sys.argv[2])
    nb_vars = int(sys.argv[3])
    minsup = int(sys.argv[4])

    positives, negatives = load_dataset(
        filename,
        nb_rows,
        list(range(1, nb_vars + 1)),
        [str(nb_vars + 1), str(nb_vars + 2)],
        negation=False,
        load_top_k=None,
        switch_signs=False,
        num_vars=100,
        load_tqdm=True,
        raw_data=True,
    )
    start_time = time()
    mistle = Mistle(positives, negatives)
    theory, compression = mistle.learn(minsup=minsup, dl_measure="ce")
    print("Total time\t\t\t\t: " + str(time() - start_time) + " seconds.")
    if theory is not None:
        print("Final theory has " + str(len(theory.clauses)) + " clauses.")
    else:
        print("Empty theory learned.")
