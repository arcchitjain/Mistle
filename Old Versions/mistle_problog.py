from problog.logic import Term, And, Or
from tqdm import tqdm

def load_animal_taxonomy():
    a = Term('a')
    b = Term('b')
    c = Term('c')
    d = Term('d')
    e = Term('e')
    f = Term('f')
    g = Term('g')
    h = Term('h')
    i = Term('i')
    j = Term('j')
    k = Term('k')
    l = Term('l')
    m = Term('m')
    n = Term('n')

    clause1_list = [b, -c, d, e, f, -g, h, -i, j, -k]
    clause2_list = [b, c, -d, e, f, g, -h, -i, j, l, -m, -n]
    clause3_list = [b, c, -d, e, f, g, -h, -i, j, -l, m, -n]
    clause4_list = [-b, c, d, e, f, -g, h, i, j, -k]
    clause5_list = [b, c, -d, e, f, -g, h, -i, j]
    clause6_list = [b, -c, d, e, f, -g, h, i, j, k, l, -m]
    clause7_list = [b, c, d, -e, f, -g, h, i, j, k, -l, m]

    c1 = Or.from_list(clause1_list)
    c2 = Or.from_list(clause2_list)
    c3 = Or.from_list(clause3_list)
    c4 = Or.from_list(clause4_list)
    c5 = Or.from_list(clause5_list)
    c6 = Or.from_list(clause6_list)
    c7 = Or.from_list(clause7_list)

    input_clauses = [c1, c2, c3, c4, c5, c6, c7]

    return input_clauses

def load_mushroom():
    schema = "e/p b/c/x/f/k/s f/g/y/s n/b/c/g/r/p/u/e/w/y t/f a/l/c/y/f/m/n/p/s a/d/f/n c/w/d b/n k/n/b/h/g/r/o/p/u/e/w/y e/t b/c/u/e/z/r f/y/k/s f/y/k/s n/b/c/g/o/p/e/w/y n/b/c/g/o/p/e/w/y p/u n/o/w/y n/o/t c/e/f/l/n/p/s/z k/n/b/h/r/o/u/w/y a/c/n/s/v/y g/l/m/p/u/w/d"
    schema_list = []
    var_dict = {}
    var_counter = 0
    for i, options in enumerate(schema.split(" ")[1:]):
        option_list = options.split('/')
        schema_list.append(len(option_list))

        var_list = []
        for j in range(len(option_list)):
            var_list.append(str(var_counter+j))
        var_counter += len(option_list)

        var_dict[i+1] = var_list

    class_dict = {}
    var_counter = 0
    for i, schema in enumerate(schema_list):
        for j in range(schema):
            class_dict[str(var_counter+j)] = i+1
        var_counter += schema

    print(schema_list)
    print(var_dict)
    print(class_dict)

    # f = open("./Data/mushroom.D90.N8124.C2.num", "rb")
    f = open("./Data/mushroom_cp4im.txt", "r")
    positive_input_clauses = []
    negative_input_clauses = []
    for line in f:
        row = str(line)[:-1].split(" ")

        clause = []
        for i in range(117):
            if str(i) in row:
                clause.append(-Term('v' + str(i)))
            else:
                clause.append(Term('v' + str(i)))

        # for var in row[:-1]:
        #     clause.append(Term('v'+str(var)))
        #     var_class = class_dict[var]
        #     for other_var in var_dict[var_class]:
        #         if other_var != var:
        #             a = Term('v' + str(other_var))
        #             clause.append(-a)

        # print(clause)

        if row[-1] == '0':
            negative_input_clauses.append(Or.from_list(clause))
        elif row[-1] == '1':
            positive_input_clauses.append(Or.from_list(clause))

    return (positive_input_clauses, negative_input_clauses)

def get_literal_length(f):
    f_str = str(f)
    f_str = f_str.replace('(', '').replace(')', '').replace(';', '').replace(',', '')
    literal_list = f_str.split(' ')
    return len(literal_list)

new_var_counter = 1

def get_new_var():
    global new_var_counter
    new_var = Term('z'+str(new_var_counter))
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

    clause1_list = clause1.to_list()
    clause2_list = clause2.to_list()

    clause1_set = set(clause1_list)
    clause2_set = set(clause2_list)

    clause_a = clause1_set - clause2_set
    clause_b = clause1_set & clause2_set
    clause_c = clause2_set - clause1_set

    if len(clause_a) == 0:
        # Apply clause absoprtion operator on (b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        return ([clause1], len(clause_b))

    elif len(clause_c) == 0:
        # Apply clause absoprtion operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3)
        return ([clause2], len(clause_b))

    elif len(clause_b) == 0:
        # Cannot compress (a1; a2; a3), (c1; c2; c3)
        print("No Overlap Found")
        return ([clause1, clause2], 0)

    elif len(clause_a) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a; b1; b2; b3), (c1; c2; c3; -a)
        a = clause_a.pop()
        clause_c.add(-a)
        return ([clause1, Or.from_list(list(clause_c))], len(clause_b) - 1)

    elif len(clause_c) == 1 and len(clause_b) > 1:
        # Apply V-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c)
        # Return (a1; a2; a3; -c), (b1; b2; b3; c)
        c = clause_c.pop()
        clause_a.add(-c)
        return ([Or.from_list(list(clause_a)), clause2], len(clause_b) - 1)

    elif len(clause_b) < 3:
        # print("Application of W-Operator is infeasible due to overlap of only " + str(len(clause_b)) + " literals.")
        return ([clause1, clause2], 0)

    else:
        # This is the case where len(clause_a) > 1, len(clause_b) > 2, and len(clause_c) > 1.
        # Apply W-operator on (a1; a2; a3; b1; b2; b3), (b1; b2; b3; c1; c2; c3)
        # Return (a1; a2; a3; -z), (b1; b2; b3; z), (c1; c2; c3, -z)
        new_var = get_new_var()
        clause_a.add(-new_var)
        clause_b.add(new_var)
        clause_c.add(-new_var)

        c_a = Or.from_list(list(clause_a))
        c_b = Or.from_list(list(clause_b))
        c_c = Or.from_list(list(clause_c))

        return ([c_a, c_b, c_c], len(clause_b) - 3)


def mistle(theory):

    print("\nInput Theory (length = " + str(get_literal_length(And.from_list(theory))) + "):")
    for clause in theory:
        print(clause)

    while True:

        # Remove redundant clauses from the theory
        theory = list(set(theory))

        # TODO: This task could be sped up
        clause_length = []
        for clause in theory:
            clause_length.append(get_literal_length(clause))

        # TODO: This task could be sped up
        index = list(range(len(clause_length)))
        index.sort(key=clause_length.__getitem__, reverse=True)
        clause_length = [clause_length[i] for i in index]
        theory = [theory[i] for i in index]

        # for clause, length in zip(theory, clause_length):
        #     print(length, clause)
        # print()

        overlap_sizes = []
        overlap_indices = []
        max_overlap_size = 0
        max_overlap_index = None

        for x, clause1 in enumerate(theory):
            for y, clause2 in enumerate(theory[x+1:]):
                if max_overlap_size > clause_length[x+y+1]:
                    break

                clause_b = set(clause1.to_list()) & set(clause2.to_list())
                overlap_size = len(clause_b)
                overlap_indices.append((x, x+y+1))
                overlap_sizes.append(overlap_size)

                if overlap_size > max_overlap_size:
                    max_overlap_size = overlap_size
                    max_overlap_index = (x, x+y+1)

                # print(str(x), str(x+y+1), str(len(clause_b)))

        clause1 = theory[max_overlap_index[0]]
        clause2 = theory[max_overlap_index[1]]

        # print("\nCompressing (" + str(clause1) + ") and (" + str(clause2) + ") for overlap of " + str(max_overlap_size) + " literals.")
        (compressed_clauses, compression) = compress_pairwise(clause1, clause2)

        if len(compressed_clauses) == 2 and compression == 0:
            # Theory cannot be compressed any further
            break
        else:
            # Continue compressing the theory
            theory.remove(clause1)
            theory.remove(clause2)
            for clause in compressed_clauses:
                theory.append(clause)

    theory_cnf = And.from_list(theory)
    print("\nResultant Theory (Length = " + str(get_literal_length(theory_cnf)) +"):")
    for clause in theory:
        print(clause)

    return theory

input_clauses = load_animal_taxonomy()
# _, input_clauses = load_mushroom()
theory = mistle(input_clauses)