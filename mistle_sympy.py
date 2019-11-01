from sympy import symbols
from tqdm import tqdm

def print_input_data_statistics(name, positive_input_clauses, negative_input_clauses, target_class, negation, load_top_k, switch_signs):
    print()
    print("Input Data Statistics:")
    print("\tDataset Name \t\t\t\t\t\t\t\t\t\t\t\t\t\t\t: " + name + "\n")

    if switch_signs:
        pos_sign = " -ve"
        neg_sign = " +ve"
    else:
        pos_sign = " +ve"
        neg_sign = " -ve"

    print("\tClass\tSign\t%age\tFrequency")
    n = len(positive_input_clauses) + len(negative_input_clauses)
    print("\t" + str(target_class[0]) +
          "\t\t" + pos_sign+
          "\t" + str(round(len(positive_input_clauses)*100/n)) +
          "%\t\t" + str(len(positive_input_clauses)))
    print("\t" + str(target_class[1]) + "\t\t" + neg_sign +
          "\t" + str(round(len(negative_input_clauses) * 100 / n)) +
          "%\t\t" + str(len(negative_input_clauses)))
    print()
    print("\tAdd absent variables as negative literals in the partial assignments \t: " + str(negation))
    if not(bool(load_top_k)):
        print("\tLoad top-k negative partial assignments \t\t\t\t\t\t\t\t: " + str(bool(load_top_k)))
    else:
        print("\tNumber of negative partial assignments loaded from the top of the file \t: " + str(load_top_k))
    print()

def load_animal_taxonomy():
    a = symbols('a')
    b = symbols('b')
    c = symbols('c')
    d = symbols('d')
    e = symbols('e')
    f = symbols('f')
    g = symbols('g')
    h = symbols('h')
    i = symbols('i')
    j = symbols('j')
    k = symbols('k')
    l = symbols('l')
    m = symbols('m')
    n = symbols('n')

    c1 = frozenset([b, ~c, d, e, f, ~g, h, ~i, j, ~k])
    c2 = frozenset([b, c, ~d, e, f, g, ~h, ~i, j, l, ~m, ~n])
    c3 = frozenset([b, c, ~d, e, f, g, ~h, ~i, j, ~l, m, ~n])
    c4 = frozenset([~b, c, d, e, f, ~g, h, i, j, ~k])
    c5 = frozenset([b, c, ~d, e, f, ~g, h, ~i, j])
    c6 = frozenset([b, ~c, d, e, f, ~g, h, i, j, k, l, ~m])
    c7 = frozenset([b, c, d, ~e, f, ~g, h, i, j, k, ~l, m])

    input_clauses = {c1, c2, c3, c4, c5, c6, c7}

    return input_clauses


def load_mushroom(negation=True, load_top_k=None, switch_signs=False):
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
    f = open("./Data/mushroom_cp4im.txt", "r")
    positive_input_clauses = set()
    negative_input_clauses = set()
    pbar = tqdm(f, total=8124)
    pbar.set_description("Reading input file")
    for line in pbar:
        row = str(line)[:-1].split(" ")

        clause = set()
        for j in range(117):
            if str(j) in row:
                clause.add(~symbols('v' + str(j)))
            elif negation:
                clause.add(symbols('v' + str(j)))

        # for var in row[:-1]:
        #     clause.append(Term('v'+str(var)))
        #     var_class = class_dict[var]
        #     for other_var in var_dict[var_class]:
        #         if other_var != var:
        #             a = Term('v' + str(other_var))
        #             clause.append(-a)

        # print(clause)

        if (not(switch_signs) and row[-1] == '0') or (switch_signs and row[-1] == '1'):
            negative_input_clauses.add(frozenset(clause))
            if load_top_k and len(negative_input_clauses) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not(switch_signs) and row[-1] == '1') or (switch_signs and row[-1] == '0'):
            positive_input_clauses.add(frozenset(clause))

    print_input_data_statistics("Mushroom", positive_input_clauses, negative_input_clauses, ['0', '1'],
                                negation, load_top_k, switch_signs)

    return (positive_input_clauses, negative_input_clauses)


def load_ionosphere(negation=True, load_top_k=None, switch_signs=False):

    f = open("./Data/ionosphere.D157.N351.C2.num", "r")
    positive_input_clauses = set()
    negative_input_clauses = set()
    pbar = tqdm(f, total=351)
    pbar.set_description("Reading input file")
    for line in pbar:
        row = str(line)[:-1].split(" ")

        clause = set()
        for j in range(117):
            if str(j) in row:
                clause.add(~symbols('v' + str(j)))
            elif negation:
                clause.add(symbols('v' + str(j)))

        if (not(switch_signs) and row[-1] == '156') or (switch_signs and row[-1] == '157'):
            negative_input_clauses.add(frozenset(clause))
            if load_top_k and len(negative_input_clauses) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not(switch_signs) and row[-1] == '157') or (switch_signs and row[-1] == '156'):
            positive_input_clauses.add(frozenset(clause))

    print_input_data_statistics("Ionosphere", positive_input_clauses, negative_input_clauses, ['156', '157'],
                                negation, load_top_k, switch_signs)

    return (positive_input_clauses, negative_input_clauses)


def load_adult(negation=True, load_top_k=None, switch_signs=False):

    f = open("./Data/adult.D97.N48842.C2.num", "r")
    positive_input_clauses = set()
    negative_input_clauses = set()
    pbar = tqdm(f, total=48842)
    pbar.set_description("Reading input file")
    for line in pbar:
        row = str(line)[:-1].split(" ")

        clause = set()
        for j in range(117):
            if str(j) in row:
                clause.add(~symbols('v' + str(j)))
            elif negation:
                clause.add(symbols('v' + str(j)))

        if (not(switch_signs) and row[-1] == '96') or (switch_signs and row[-1] == '97'):
            negative_input_clauses.add(frozenset(clause))
            if load_top_k and len(negative_input_clauses) == load_top_k:
                # Top k negative clauses have been loaded already
                break
        elif (not(switch_signs) and row[-1] == '97') or (switch_signs and row[-1] == '96'):
            positive_input_clauses.add(frozenset(clause))

    print_input_data_statistics("Adult", positive_input_clauses, negative_input_clauses, ['96','97'],
                                negation, load_top_k, switch_signs)

    return (positive_input_clauses, negative_input_clauses)



def get_literal_length(theory):

    length = 0
    for clause in theory:
        length += len(clause)

    return length


new_var_counter = 1


def get_new_var():
    global new_var_counter
    new_var = symbols('z'+str(new_var_counter))
    new_var_counter += 1
    return new_var


def compress_pairwise(clause1, clause2):
    """
    Compress clause1 and clause2
    :param clause1:
    :param clause2:
    :return:
        A list of the new clauses obtained after compression
        The decrease in literal length achieved during compression
    """

    clause_a = set(clause1 - clause2)
    clause_b = set(clause1 & clause2)
    clause_c = set(clause2 - clause1)

    if len(clause_a) == 0:
        # Apply clause absoprtion operator on (b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        return ({clause1}, len(clause_b))

    elif len(clause_c) == 0:
        # Apply clause absoprtion operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3)
        return ({clause2}, len(clause_b))

    elif len(clause_b) == 0:
        # Cannot compress (a1; a2; a3), (c1; c2; c3)
        print("No Overlap Found")
        return ({clause1, clause2}, 0)

    elif len(clause_a) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a; b1; b2; b3), (c1; c2; c3; -a)
        a = clause_a.pop()
        clause_c.add(~a)
        return ({clause1, frozenset(clause_c)}, len(clause_b) - 1)

    elif len(clause_c) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
        # Return (a1; a2; a3; -c), (b1; b2; b3; c)
        c = clause_c.pop()
        clause_a.add(~c)
        return ({frozenset(clause_a), clause2}, len(clause_b) - 1)

    elif len(clause_b) < 3:
        # print("Application of W-Operator is infeasible due to overlap of only " + str(len(clause_b)) + " literals.")
        return ({clause1, clause2}, 0)

    else:
        # This is the case where len(clause_a) > 1, len(clause_b) > 2, and len(clause_c) > 1.
        # Apply W-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a1; a2; a3; -z), (b1; b2; b3; z), (c1; c2; c3, -z)
        new_var = get_new_var()
        clause_a.add(~new_var)
        clause_b.add(new_var)
        clause_c.add(~new_var)

        return ({frozenset(clause_a), frozenset(clause_b), frozenset(clause_c)}, len(clause_b) - 3)

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
        overlap_matrix.append([0]*n)
        for j, clause2 in enumerate(theory[i+1:]):
            overlap_matrix[i][i+j+1] = len(clause1 & clause2)

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
        for j, overlap in enumerate(row[i+1:]):
            if overlap > max_overlap_size:
                max_overlap_size = overlap
                max_overlap_indices = (i, j)

    return max_overlap_indices, max_overlap_size


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
        for y, clause2 in enumerate(theory[x + 1:end]):
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
    for i in range(n-2):
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
            # Insert element at/before (i + 1) if clause_length[i] >= len(clause) >= clause_length[i + 1]
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

    new_overlap_row = [0]*(len(theory)+1)
    for j, clause2 in enumerate(theory[index+1:]):
        new_overlap_row[index+j+1] = len(clause & clause2)

    overlap_matrix[index:index] = [new_overlap_row]

    # print_2d(overlap_matrix)

    theory[index:index] = [clause]
    clause_length[index:index] = [len(clause)]

    return theory, clause_length, overlap_matrix

def mistle(theory):

    input_length = get_literal_length(theory)
    print("\nInput Theory:\n\tLiteral Length = " + str(input_length) + "\n\tNumber of clauses = " + str(
        len(theory)))


    # print_2d(theory)

    # Remove redundant clauses from the theory
    theory = list(theory)
    theory, clause_length = sort_theory(theory)

    overlap_matrix = get_overlap_matrix(theory)

    overlap_size = None
    compression_counter = 0
    pbar = tqdm(total = int(1.1 * len(theory) + 100))
    pbar.set_description("Compressing Clauses")
    while True:
        prev_overlap_size = overlap_size
        max_overlap_indices, overlap_size = select_clauses_2(theory, clause_length, prev_overlap_size)

        clause1 = theory[max_overlap_indices[0]]
        clause2 = theory[max_overlap_indices[1]]

        # print("\nCompressing (" + str(clause1) + ") and (" + str(clause2) + ") for overlap of " + str(max_overlap_size) + " literals.")
        (compressed_clauses, compression) = compress_pairwise(clause1, clause2)
        # print(compression_counter, compression)
        compression_counter += 1

        if len(compressed_clauses) == 2 and compression == 0:
            # Theory cannot be compressed any further
            break
        else:
            # Continue compressing the theory
            theory, clause_length, overlap_matrix = delete_clause(theory, clause_length, overlap_matrix, max_overlap_indices)

            # Insert clauses from compressed_clauses at appropriate lengths so that the theory remains sorted
            # theory |= compressed_clauses
            for clause in compressed_clauses:
                theory, clause_length, overlap_matrix = insert_clause(theory, clause_length, overlap_matrix, clause)

        # print_2d(theory)
        pbar.update(1)

    global new_var_counter
    output_length = get_literal_length(theory)
    print("\nResultant Theory:\n\tLiteral Length = " + str(output_length) +
          "\n\tNumber of clauses = " + str(len(theory)) +
          "\n\tNumber of invented predicates = " + str(new_var_counter - 1) +
          "\n\t% Compression = " + str(round(output_length * 100/input_length, 2)) + "%")
    # print_2d(theory)

    return theory

# input_clauses = load_animal_taxonomy()
# _, input_clauses = load_mushroom(negation = True, load_top_k = None)
_, input_clauses = load_mushroom(negation = False, load_top_k = 1000)
_, input_clauses = load_adult(negation = False, load_top_k = None)
_, input_clauses = load_ionosphere(negation = True, load_top_k = None, switch_signs=True)
# theory = mistle(input_clauses)