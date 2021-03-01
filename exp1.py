from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
import random
import csv
import numpy as np
import sys
from copy import copy

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


def generate_complex_theory(alphabet_size=14):
    # clause_size = (1, 4)
    clause_size = (1, alphabet_size)
    # nb_clauses = (10, alphabet_size)
    nb_clauses = (2, min(10, alphabet_size))
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


def compare_2_theories(
    actual_clauses, clauses_plus, clauses_minus, alphabet_size, pa_set=None
):
    if clauses_plus is np.nan and clauses_minus is np.nan:
        return np.nan, np.nan, np.nan, np.nan

    similarity_plus = 0
    similarity_minus = 0
    similarity_common = 0
    common_total = 0

    if pa_set is None or len(pa_set) == 0:
        denominator = 2 ** alphabet_size
        for counter in range(0, 2 ** alphabet_size):
            binary_counter = format(counter, "b")
            char_counter = binary_counter.rjust(alphabet_size, "0")
            pa = set()
            for i, char in enumerate(char_counter):
                if char == "0":
                    pa.add(-i - 1)
                elif char == "1":
                    pa.add(i + 1)

            a = check_pa_satisfiability(pa, actual_clauses, pa_is_complete=True)
            b = check_pa_satisfiability(pa, clauses_plus, pa_is_complete=True)
            c = not (check_pa_satisfiability(pa, clauses_minus, pa_is_complete=True))

            if a == b:
                similarity_plus += 1
            if a == c:
                similarity_minus += 1
            if b == c:
                if a == b:
                    similarity_common += 1
                common_total += 1

    else:
        denominator = len(pa_set)
        for pa in pa_set:
            a = check_pa_satisfiability(pa, actual_clauses, pa_is_complete=True)
            b = check_pa_satisfiability(pa, clauses_plus, pa_is_complete=True)
            c = not (check_pa_satisfiability(pa, clauses_minus, pa_is_complete=True))
            if a == b:
                similarity_plus += 1
            if a == c:
                similarity_minus += 1
            if b == c:
                if a == b:
                    similarity_common += 1
                common_total += 1

    if common_total > 0:
        similarity_common = similarity_common / common_total
    else:
        similarity_common = np.nan
    return (
        similarity_plus / denominator,
        similarity_minus / denominator,
        similarity_common,
        common_total / denominator,
    )


###############################################################################
# Plot 0: Which Mistle Theory to chose from?
###############################################################################


iterations = int(sys.argv[1])
missingness_parameter = float(sys.argv[2])
alphabet_size = int(sys.argv[3])
data_size = int(sys.argv[4])
version = int(sys.argv[5])
# iterations = 10
# missingness_parameters = [0.1]
# version = 3
balance = False

complex_theory_list = []
pos_list = []
neg_list = []
unique_positives = []
unique_negatives = []
default_accuracy_list = []
nb_clases_list = []
mean_nb_literals = []
pa_set = set()
for counter in range(0, 2 ** alphabet_size):
    binary_counter = format(counter, "b")
    char_counter = binary_counter.rjust(alphabet_size, "0")
    pa = set()
    for i, char in enumerate(char_counter):
        if char == "0":
            pa.add(-i - 1)
        elif char == "1":
            pa.add(i + 1)
    pa_set.add(frozenset(pa))

for i in range(iterations):
    # print(i)
    actual_theory = []
    positives = set()
    negatives = set()
    while len(positives) == 0 or len(negatives) == 0:
        actual_theory = []
        while actual_theory == []:
            actual_theory = generate_complex_theory(alphabet_size)
            if solve([tuple(clause) for clause in actual_theory]) == "UNSAT":
                print("Generated theory is not SAT\t: " + str(actual_theory))
                actual_theory = []
        for pa in pa_set:
            if check_pa_satisfiability(pa, actual_theory, pa_is_complete=True):
                positives.add(frozenset(pa))
            else:
                negatives.add(frozenset(pa))
        if len(positives) == 0:
            print("No positives detected")
        if len(negatives) == 0:
            print("No negatives detected")

    complex_theory_list.append(actual_theory)
    pos_list.append(positives)
    neg_list.append(negatives)
    unique_positives.append(len(positives))
    unique_negatives.append(len(negatives))
    default_accuracy_list.append(
        max(len(negatives), len(positives)) / (len(positives) + len(negatives))
    )
    nb_clases_list.append(len(actual_theory))
    mean_nb_literals.append(mean([len(clause) for clause in actual_theory]))

print("complex_theory_list\t: ")
for i, t in enumerate(complex_theory_list):
    print(str(i), str(t))
print("unique_positives\t\t: " + str(unique_positives))
print("unique_negatives\t\t: " + str(unique_negatives))
print("default_accuracy_list\t: " + str(default_accuracy_list))
print("nb_clases_list\t\t\t: " + str(nb_clases_list))
print("mean_nb_literals\t\t: " + str(mean_nb_literals))

# missingness_parameters = [0, 0.1, 0.2, 0.3, 0.4, 0.5]
similarity_plus_list = []
similarity_minus_list = []
similarity_mf_list = []
similarity_comp_list = []
similarity_mdl_list = []
similarity_common_list = []

used_theory_list = []
t_plus_list = []
t_minus_list = []
mf_list = []
mdl_list = []
comp_list = []
t_plus_dl_list = []
t_minus_dl_list = []
t_plus_comp_list = []
t_minus_comp_list = []
samp_positives_list = []
samp_negatives_list = []
samp_unique_positives_list = []
samp_unique_negatives_list = []
overlap_size_list = []

# simple_theory_list = []
# for i in range(iterations):
#     simple_theory_list.append(generate_theory(alphabet_size))

for i in range(iterations):
    print(
        "\n\n==================== New Iteration [" + str(i) + "] ===================="
    )
    actual_theory = complex_theory_list[i]
    # actual_theory = simple_theory_list.pop()
    theory_object = GeneratedTheory(actual_theory)
    generator = TheoryNoisyGeneratorOnDataset(
        theory_object, data_size, missingness_parameter
    )
    positives, negatives = generator.generate_dataset()

    samp_positives = len(positives)
    samp_negatives = len(negatives)
    samp_unique_positives = len(set(positives))
    samp_unique_negatives = len(set(negatives))

    if samp_positives == 0:
        print(i, "Sampled Positives are empty")
    if samp_negatives == 0:
        print(i, "Sampled Negatives are empty")

    if balance:
        # Make sure that the number of unique positives is equal to the number of unique negatives
        if samp_unique_negatives < samp_unique_positives:
            positives = copy(random.sample(set(positives), k=samp_unique_negatives))
            negatives = list(set(negatives))
        if samp_unique_positives < samp_unique_negatives:
            positives = list(set(positives))
            negatives = copy(random.sample(set(negatives), k=samp_unique_positives))

        samp_positives = len(positives)
        samp_negatives = len(negatives)
        samp_unique_positives = len(set(positives))
        samp_unique_negatives = len(set(negatives))

        assert (
            samp_positives
            == samp_negatives
            == samp_unique_positives
            == samp_unique_negatives
        )

    samp_positives_list.append(samp_positives)
    samp_negatives_list.append(samp_negatives)
    samp_unique_positives_list.append(samp_unique_positives)
    samp_unique_negatives_list.append(samp_unique_negatives)

    used_theory_list.append(actual_theory)
    print("\n========== Learning T+ [" + str(i) + "] ==========")
    t_plus, comp_plus = Mistle(positives, negatives).learn(k=5000)
    # t_plus, comp_plus = Mistle(positives, negatives).learn(minsup=0.02)
    print("\n========== Learning T- [" + str(i) + "] ==========")
    t_minus, comp_minus = Mistle(negatives, positives).learn(k=5000)
    # t_minus, comp_minus = Mistle(negatives, positives).learn(minsup=0.02)

    if t_plus is not None:
        t_plus_list.append([list(t) for t in get_clauses(t_plus)])
        t_plus_dl_list.append(t_plus.dl)
        dl_plus = t_plus.dl
    else:
        t_plus_list.append(np.nan)
        t_plus_dl_list.append(np.nan)
        dl_plus = np.nan

    if t_minus is not None:
        t_minus_list.append([list(t) for t in get_clauses(t_minus)])
        t_minus_dl_list.append(t_minus.dl)
        dl_minus = t_minus.dl
    else:
        t_minus_list.append(np.nan)
        t_minus_dl_list.append(np.nan)
        dl_minus = np.nan

    t_plus_comp_list.append(comp_plus)
    t_minus_comp_list.append(comp_minus)

    similarity_plus, similarity_minus, similarity_common, overlap_size = compare_2_theories(
        actual_theory, get_clauses(t_plus), get_clauses(t_minus), alphabet_size
    )
    similarity_plus_list.append(similarity_plus)
    similarity_minus_list.append(similarity_minus)
    similarity_common_list.append(similarity_common)
    overlap_size_list.append(overlap_size)

    if similarity_minus is np.nan and similarity_plus is np.nan:
        mf_list.append(np.nan)
        comp_list.append(np.nan)
        mdl_list.append(np.nan)
        similarity_comp_list.append(np.nan)
        similarity_mdl_list.append(np.nan)
        similarity_mf_list.append(np.nan)
    else:
        if samp_unique_positives > samp_unique_negatives:
            mf_list.append("T-")
            similarity_mf_list.append(similarity_minus)
        else:
            mf_list.append("T+")
            similarity_mf_list.append(similarity_plus)

        if comp_plus > comp_minus:
            similarity_comp_list.append(similarity_plus)
            comp_list.append("T+")
        else:
            similarity_comp_list.append(similarity_minus)
            comp_list.append("T-")

        if dl_plus < dl_minus:
            similarity_mdl_list.append(similarity_plus)
            mdl_list.append("T+")
        else:
            similarity_mdl_list.append(similarity_minus)
            mdl_list.append("T-")

print("\nused_theory_list")
for i in range(iterations):
    print(used_theory_list[i])

print("\nt_plus_list")
for i in range(iterations):
    print(t_plus_list[i])

print("\nt_minus_list")
for i in range(iterations):
    print(t_minus_list[i])

print("\nsimilarity_plus_list\n" + str(similarity_plus_list))
print("similarity_minus_list\n" + str(similarity_minus_list))
print("similarity_mf_list\n" + str(similarity_mf_list))
print("similarity_comp_list\n" + str(similarity_comp_list))
print("similarity_mdl_list\n" + str(similarity_mdl_list))
print("similarity_common_list\n" + str(similarity_common_list))
print("mf_list\n" + str(mf_list))
print("comp_list\n" + str(comp_list))
print("mdl_list\n" + str(mdl_list))
print("t_plus_comp_list\n" + str(t_plus_comp_list))
print("t_minus_comp_list\n" + str(t_minus_comp_list))
print("t_plus_dl_list\n" + str(t_plus_dl_list))
print("t_minus_dl_list\n" + str(t_minus_dl_list))
print("samp_positives_list\n" + str(samp_positives_list))
print("samp_negatives_list\n" + str(samp_negatives_list))
print("samp_unique_positives_list\n" + str(samp_unique_positives_list))
print("samp_unique_negatives_list\n" + str(samp_unique_negatives_list))


with open(
    "Logs/exp1_"
    + str(iterations)
    + "_"
    + str(missingness_parameter)
    + "_"
    + str(alphabet_size)
    + "_"
    + str(data_size)
    + "_v"
    + str(version)
    + ".csv",
    "w",
) as f:
    write = csv.writer(f)
    write.writerow(
        [
            "Used Theory",
            "Total Unique Positives",
            "Total Unique Negatives",
            "Default Accuracy",
            "# Clauses",
            "Mean # Literals",
            "T+",
            "T-",
            "Similarity of T+",
            "Similarity of T-",
            "Similarity of T_MF",
            "Similarity of T_Comp",
            "Similarity of T_MDL",
            "Similarity - Common",
            "Overlap Size",
            "MF Theory",
            "Comp Theory",
            "MDL Theory",
            "Compression of T+",
            "Compression of T-",
            "DL of T+",
            "DL of T-",
            "# Sampled Positives",
            "# Sampled Negatives",
            "# Unique Sampled Positives",
            "# Unique Sampled Negatives",
        ]
    )

    for i in range(iterations):
        row = [str(used_theory_list[i])]
        row.append(str(unique_positives[i]))
        row.append(str(unique_negatives[i]))
        row.append(str(default_accuracy_list[i]))
        row.append(str(nb_clases_list[i]))
        row.append(str(mean_nb_literals[i]))
        row.append(str(t_plus_list[i]))
        row.append(str(t_minus_list[i]))
        row.append(str(similarity_plus_list[i]))
        row.append(str(similarity_minus_list[i]))
        row.append(str(similarity_mf_list[i]))
        row.append(str(similarity_comp_list[i]))
        row.append(str(similarity_mdl_list[i]))
        row.append(str(similarity_common_list[i]))
        row.append(str(overlap_size_list[i]))
        row.append(str(mf_list[i]))
        row.append(str(comp_list[i]))
        row.append(str(mdl_list[i]))
        row.append(str(t_plus_comp_list[i]))
        row.append(str(t_minus_comp_list[i]))
        row.append(str(t_plus_dl_list[i]))
        row.append(str(t_minus_dl_list[i]))
        row.append(str(samp_positives_list[i]))
        row.append(str(samp_negatives_list[i]))
        row.append(str(samp_unique_positives_list[i]))
        row.append(str(samp_unique_negatives_list[i]))

        write.writerow(row)


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


# data_sizes = [100, 150, 200, 250, 300, 350, 400, 450, 500]
# data_sizes = [20, 30, 40, 50, 60, 70, 80, 90, 100]
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

# s_list = []
# for i in range(iterations):
#     s_list.append(plot3(alphabet_sizes=alphabet_sizes))
#
# s_list = np.array(s_list)
# mean_similarity_list = []
# lstd_dev_similarity_list = []
# ustd_dev_similarity_list = []
# std_dev_similarity_list = []
# for i, ds in enumerate(alphabet_sizes):
#     m = mean(s_list[:, i])
#     std_dev = stdev(s_list[:, i])
#     mean_similarity_list.append(m)
#     std_dev_similarity_list.append(std_dev)
#     lstd_dev_similarity_list.append(max(m - std_dev, 0))
#     ustd_dev_similarity_list.append(min(m + std_dev, 1))
#
# print("alphabet_sizes = " + str(alphabet_sizes))
# print("mean_similarity_list = " + str(mean_similarity_list))
# print("std_dev_similarity_list = " + str(std_dev_similarity_list))
# print("lstd_dev_similarity_list = " + str(lstd_dev_similarity_list))
# print("ustd_dev_similarity_list = " + str(ustd_dev_similarity_list))

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
