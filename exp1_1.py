from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
import random
import numpy as np
from mistle_v2 import *
import os
import matplotlib
import matplotlib.pyplot as plt
from statistics import mean, stdev

plt.style.use("seaborn")

matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=28)
matplotlib.rc("axes", titlesize=28)
matplotlib.rc("axes", labelsize=28)
matplotlib.rc("xtick", labelsize=28)
matplotlib.rc("ytick", labelsize=28)
matplotlib.rc("legend", fontsize=28)
matplotlib.rc("figure", titlesize=28)
lylim = 0.5
uylim = 1.0

seed = 0
random.seed(seed)
start_time = time()
version = 4


def generate_theory(alphabet_size):

    vars = []
    for i in range(1, alphabet_size + 1):
        if random.random() < 0.5:
            vars.append(i)
        else:
            vars.append(-i)

    nb_clauses = random.choice(range(2, alphabet_size - 1))
    vars += ["#"] * nb_clauses

    random.shuffle(vars)

    cnf = []
    clause = []
    for i, symbol in enumerate(vars):
        if symbol == "#" or i == len(vars) - 1:
            if len(clause) > 0:
                cnf.append(clause)
            clause = []
            continue
        else:
            clause.append(symbol)

    print("Generated CNF\t: " + str(cnf))
    return cnf


def generate_complex_theory(alphabet_size=14, clause_size=(2, 14), nb_clauses=(2, 10)):
    cnf = []
    all_vars = list(range(1, alphabet_size + 1))
    sampled_nb_clauses = random.choice(range(*nb_clauses))
    for clause_id in range(sampled_nb_clauses):
        sampled_clause_size = random.choice(range(*clause_size))
        sampled_vars = random.sample(all_vars, k=sampled_clause_size)
        sampled_signs = random.choices((1, -1), k=sampled_clause_size)
        sampled_clause = [
            sampled_vars[i] * sampled_signs[i] for i in range(sampled_clause_size)
        ]
        cnf.append(sampled_clause)
    return cnf


def compare_theories(actual_clauses, learned_clauses, alphabet_size):
    similarity = 0
    for counter in range(0, 2 ** alphabet_size):
        binary_counter = format(counter, "b")
        char_counter = binary_counter.rjust(alphabet_size, "0")
        pa = set()
        for i, char in enumerate(char_counter):
            if char == "0":
                pa.add(-i - 1)
            elif char == "1":
                pa.add(i + 1)

        if check_pa_satisfiability(
            pa, actual_clauses, pa_is_complete=True
        ) == check_pa_satisfiability(pa, learned_clauses, pa_is_complete=True):
            similarity += 1

    return similarity / (2 ** alphabet_size)


###############################################################################
# Plot 1: Data Size
###############################################################################


def plot1(
    data_sizes, alphabet_size=14, missingness_parameter=0.1, dl="me", minsup=0.02
):
    similarity_list = []
    for data_size in data_sizes:
        positives = []
        negatives = []
        while len(positives) == 0 or len(negatives) == 0:
            actual_theory = []
            while actual_theory == []:
                actual_theory = generate_complex_theory(
                    alphabet_size, clause_size=(2, alphabet_size)
                )
                if solve([tuple(clause) for clause in actual_theory]) == "UNSAT":
                    print("Generated theory is not SAT\t: " + str(actual_theory))
                    actual_theory = []
            theory_object = GeneratedTheory(actual_theory)
            generator = TheoryNoisyGeneratorOnDataset(
                theory_object, data_size, missingness_parameter
            )
            positives, negatives = generator.generate_dataset()

        learned_theory, _ = Mistle(positives, negatives).learn(
            dl_measure=dl, minsup=minsup
        )
        learned_clauses = get_clauses(learned_theory)

        similarity = compare_theories(actual_theory, learned_clauses, alphabet_size)
        similarity_list.append(similarity)

    return similarity_list


data_sizes = [100, 150, 200, 250, 300, 350, 400, 450, 500]
# data_sizes = [20, 30, 40, 50, 60, 70, 80, 90, 100]
iterations = 100

s_list = []
for i in range(iterations):
    s_list.append(plot1(data_sizes=data_sizes))

s_list = np.array(s_list)
mean_similarity_list = []
lstd_dev_similarity_list = []
ustd_dev_similarity_list = []
std_dev_similarity_list = []
for i, ds in enumerate(data_sizes):
    m = mean(s_list[:, i])
    std_dev = stdev(s_list[:, i])
    mean_similarity_list.append(m)
    std_dev_similarity_list.append(std_dev)
    lstd_dev_similarity_list.append(max(m - std_dev, 0))
    ustd_dev_similarity_list.append(min(m + std_dev, 1))

print("data_sizes = " + str(data_sizes))
print("mean_similarity_list = " + str(mean_similarity_list))
print("std_dev_similarity_list = " + str(std_dev_similarity_list))
print("lstd_dev_similarity_list = " + str(lstd_dev_similarity_list))
print("ustd_dev_similarity_list = " + str(ustd_dev_similarity_list))