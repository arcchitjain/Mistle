from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
import random

import numpy as np

# from time import time
from mistle_v2 import *
import os
import matplotlib
import matplotlib.pyplot as plt

# import mplcyberpunk
from statistics import mean, stdev

# plt.style.use("cyberpunk")
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
# for param in ['figure.facecolor', 'axes.facecolor', 'savefig.facecolor']:
#     plt.rcParams[param] = '0.9'  # very light grey
#
# for param in ['text.color', 'axes.labelcolor', 'xtick.color', 'ytick.color']:
#     plt.rcParams[param] = '#212946'  # bluish dark grey

# sns.set(color_codes=True)
# current_palette = sns.color_palette("pastel")
# sns.palplot(current_palette)
# sns.set_context("paper")

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


def compare_theories(actual_clauses, learned_clauses, alphabet_size):
    similarity = 0
    actual_tuples = [tuple(clause) for clause in actual_clauses]
    learned_tuples = [tuple(clause) for clause in learned_clauses]

    for counter in range(0, 2 ** alphabet_size):
        binary_counter = format(counter, "b")
        char_counter = binary_counter.rjust(alphabet_size, "0")

        new_actual_tuples = copy(actual_tuples)
        new_learned_tuples = copy(learned_tuples)
        for i, char in enumerate(char_counter):
            if char == "0":
                new_actual_tuples.append((-i - 1,))
                new_learned_tuples.append((-i - 1,))
            elif char == "1":
                new_actual_tuples.append((i + 1,))
                new_learned_tuples.append((i + 1,))

        if solve(new_actual_tuples) == solve(new_learned_tuples):
            similarity += 1

    return similarity / (2 ** alphabet_size)


###############################################################################
# Plot 1: Data Size
###############################################################################


def plot1(data_sizes, alphabet_size=10, missingness_parameter=0.1, dl="me", minsup=1):
    similarity_list = []
    for data_size in data_sizes:
        positives = []
        negatives = []
        while len(positives) == 0 or len(negatives) == 0:
            actual_theory = generate_theory(alphabet_size)
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

    # print(similarity_list)
    return similarity_list


data_sizes = [100, 150, 200, 250, 300, 350, 400, 450, 500]
# iterations = 100
#
# s_list = []
# for i in range(iterations):
#     s_list.append(plot1(data_sizes=data_sizes))
#
# s_list = np.array(s_list)
# mean_similarity_list = []
# lstd_dev_similarity_list = []
# ustd_dev_similarity_list = []
# std_dev_similarity_list = []
# for i, ds in enumerate(data_sizes):
#     m = mean(s_list[:, i])
#     std_dev = stdev(s_list[:, i])
#     mean_similarity_list.append(m)
#     std_dev_similarity_list.append(std_dev)
#     lstd_dev_similarity_list.append(max(m - std_dev, 0))
#     ustd_dev_similarity_list.append(min(m + std_dev, 1))
#
# print(mean_similarity_list)
# print(std_dev_similarity_list)
# print(lstd_dev_similarity_list)
# print(ustd_dev_similarity_list)
#
# mean_similarity_list = [
#     0.970703125,
#     0.9612630208333334,
#     0.9879557291666666,
#     0.9887152777777778,
#     0.98828125,
#     0.9928385416666666,
#     0.9780815972222222,
#     0.9621310763888888,
#     0.9930555555555556,
# ]
# std_dev_similarity_list = [
#     0.029402478940087105,
#     0.04442822769796063,
#     0.019167760182021994,
#     0.015348747411615621,
#     0.01581458470771645,
#     0.011293978033418337,
#     0.021064744637930965,
#     0.042982003065836064,
#     0.007428368298986997,
# ]
# lstd_dev_similarity_list = [
#     0.9413006460599129,
#     0.9168347931353727,
#     0.9687879689846446,
#     0.9733665303661622,
#     0.9724666652922835,
#     0.9815445636332483,
#     0.9570168525842913,
#     0.9191490733230527,
#     0.9856271872565686,
# ]
# ustd_dev_similarity_list = [1, 1, 1, 1, 1, 1, 0.9991463418601532, 1, 1]
#
# plt.figure(1)
# plt.ylabel("Similarity")
# plt.xlabel("Data size")
# # plt.title("Mistle is able to learn the actual theory when data size is varied")
# plt.plot(data_sizes, mean_similarity_list)
#
# plt.ylim(bottom=lylim, top=uylim)
# # plt.xlim(left=95, right=505)
# # mplcyberpunk.add_glow_effects()
# # mplcyberpunk.make_lines_glow(plt)
# # mplcyberpunk.add_underglow()
# plt.savefig("Experiments/exp1_plot1_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()


###############################################################################
# Plot 2: Missingness Paramter
###############################################################################


def plot2(missingness_parameters, data_size=400, alphabet_size=10, dl="me", minsup=1):

    similarity_list = []

    for missingness_parameter in missingness_parameters:
        positives = []
        negatives = []
        while len(positives) == 0 or len(negatives) == 0:
            actual_theory = generate_theory(alphabet_size)
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

    # print(similarity_list)
    return similarity_list


missingness_parameters = [0, 0.1, 0.2, 0.3, 0.4, 0.5]
# # missingness_parameters = [0, 0.1, 0.2]
po_paramaters = []
for m in missingness_parameters:
    po_paramaters.append(1 - m)
#
# iterations = 100
#
# s_list = []
# for i in range(iterations):
#     s_list.append(plot2(missingness_parameters=missingness_parameters))
#
# s_list = np.array(s_list)
# mean_similarity_list = []
# lstd_dev_similarity_list = []
# ustd_dev_similarity_list = []
# std_dev_similarity_list = []
# for i, ds in enumerate(missingness_parameters):
#     m = mean(s_list[:,i])
#     std_dev = stdev(s_list[:,i])
#     mean_similarity_list.append(m)
#     std_dev_similarity_list.append(std_dev)
#     lstd_dev_similarity_list.append(max(m - std_dev, 0))
#     ustd_dev_similarity_list.append(min(m + std_dev, 1))
#
# print(mean_similarity_list)
# print(std_dev_similarity_list)
# print(lstd_dev_similarity_list)
# print(ustd_dev_similarity_list)

# mean_similarity_list = [
#     0.9915364583333334,
#     0.9873046875,
#     0.9588216145833334,
#     0.9892578125,
#     0.9934895833333334,
#     0.9938151041666666,
# ]
# std_dev_similarity_list = [
#     0.012066245438053801,
#     0.015354096880494958,
#     0.0701180489008137,
#     0.01307281291459493,
#     0.007246769327783531,
#     0.00613500989030716,
# ]
# lstd_dev_similarity_list = [
#     0.9794702128952796,
#     0.9719505906195051,
#     0.8887035656825196,
#     0.976184999585405,
#     0.9862428140055498,
#     0.9876800942763595,
# ]
# ustd_dev_similarity_list = [1, 1, 1, 1, 1, 0.9999501140569738]
#
#
# plt.figure(2)
# plt.ylabel("Similarity")
# plt.xlabel("Partial Observability Parameter")
# # plt.title(
# #     "Mistle is able to learn the actual theory when missingness paramter is varied"
# # )
# plt.plot(po_paramaters, mean_similarity_list)
# plt.fill_between(
#     po_paramaters, lstd_dev_similarity_list, ustd_dev_similarity_list, alpha=0.3
# )
# plt.ylim(bottom=lylim, top=uylim)
# plt.savefig("Experiments/exp1_plot2_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()


###############################################################################
# Plot 3: Alphabet Size
###############################################################################


def plot3(
    alphabet_sizes, data_size=400, missingness_parameter=0.1, dl="me", minsup=0.02
):
    similarity_list = []

    for alphabet_size in alphabet_sizes:
        positives = []
        negatives = []
        while len(positives) == 0 or len(negatives) == 0:
            actual_theory = generate_theory(alphabet_size)
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

    # print(similarity_list)
    return similarity_list


alphabet_sizes = [4, 6, 8, 10, 12, 14, 16]
iterations = 100

s_list = []
for i in range(iterations):
    s_list.append(plot3(alphabet_sizes=alphabet_sizes))

s_list = np.array(s_list)
mean_similarity_list = []
lstd_dev_similarity_list = []
ustd_dev_similarity_list = []
std_dev_similarity_list = []
for i, ds in enumerate(alphabet_sizes):
    m = mean(s_list[:, i])
    std_dev = stdev(s_list[:, i])
    mean_similarity_list.append(m)
    std_dev_similarity_list.append(std_dev)
    lstd_dev_similarity_list.append(max(m - std_dev, 0))
    ustd_dev_similarity_list.append(min(m + std_dev, 1))

print("alphabet_sizes = " + str(alphabet_sizes))
print("mean_similarity_list = " + str(mean_similarity_list))
print("std_dev_similarity_list = " + str(std_dev_similarity_list))
print("lstd_dev_similarity_list = " + str(lstd_dev_similarity_list))
print("ustd_dev_similarity_list = " + str(ustd_dev_similarity_list))

# mean_similarity_list = [1.0, 1.0, 0.9895833333333334]
# std_dev_similarity_list = [0.0, 0.0, 0.018042195912175804]
# lstd_dev_similarity_list = [1.0, 1.0, 0.9715411374211576]
# ustd_dev_similarity_list = [1.0, 1.0, 1]

# mean_similarity_list = [
#     0.995849609375,
#     0.98212890625,
#     0.98037109375,
#     0.9890625,
#     0.99892578125,
# ]
# std_dev_similarity_list = [
#     0.006142453918656799,
#     0.02228403501506705,
#     0.02791414695489953,
#     0.017825944076725993,
#     0.002402026147704852,
# ]
# lstd_dev_similarity_list = [
#     0.9897071554563432,
#     0.959844871234933,
#     0.9524569467951004,
#     0.971236555923274,
#     0.9965237551022951,
# ]
# ustd_dev_similarity_list = [1, 1, 1, 1, 1]
#
# plt.figure(3)
# plt.ylabel("Similarity")
# plt.xlabel("Number of variables")
# # plt.title("Mistle is able to learn the actual theory when alphabet size is varied")
# plt.plot(alphabet_sizes, mean_similarity_list)
# plt.fill_between(
#     alphabet_sizes, lstd_dev_similarity_list, ustd_dev_similarity_list, alpha=0.3
# )
# plt.ylim(bottom=lylim, top=uylim)
# plt.savefig("Experiments/exp1_plot3_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()
