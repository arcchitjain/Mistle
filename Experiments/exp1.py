from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
import random
import numpy as np
from time import time
from mistle_v2 import *
import os
import matplotlib.pyplot as plt
import mplcyberpunk

plt.style.use("cyberpunk")
seed = 0
random.seed(seed)
np.random.seed(seed)
os.chdir("..")
start_time = time()
version = 1


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
        b = format(counter, "b")
        c = b.rjust(alphabet_size, "0")

        new_actual_tuples = actual_tuples
        new_learned_tuples = learned_tuples
        for i, char in enumerate(c):
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

alphabet_size = 10
data_sizes = [100, 200, 300, 400, 500]
missingness_parameter = 0.1
dl = "me"
support = 1
similarity_list = []

for data_size in data_sizes:
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)
    generator = TheoryNoisyGeneratorOnDataset(
        theory_object, data_size, missingness_parameter
    )
    positives, negatives = generator.generate_dataset()

    mistle = Mistle(positives, negatives)
    learned_theory, compression = mistle.learn(dl_measure=dl, minsup=support)
    learned_clauses = get_clauses(learned_theory)

    similarity = compare_theories(actual_theory, learned_clauses, alphabet_size)
    similarity_list.append(similarity)


print(similarity_list)

plt.figure(1)
plt.ylabel("Similarity")
plt.xlabel("Number of clauses in data")
plt.title("Mistle is able to learn the actual theory when data size is varied")
plt.plot(data_sizes, similarity_list, marker="o")
plt.ylim(bottom=0.975, top=1)
mplcyberpunk.add_glow_effects()
# mplcyberpunk.make_lines_glow(plt)
# mplcyberpunk.add_underglow(plt)
plt.savefig("Experiments/exp1_plot1_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()


###############################################################################
# Plot 2: Missingness Paramter
###############################################################################

alphabet_size = 10
data_size = 400
missingness_parameters = [0, 0.1, 0.2, 0.3, 0.4, 0.5]
dl = "me"
support = 1
similarity_list = []

for missingness_parameter in missingness_parameters:
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)
    generator = TheoryNoisyGeneratorOnDataset(
        theory_object, data_size, missingness_parameter
    )
    positives, negatives = generator.generate_dataset()

    mistle = Mistle(positives, negatives)
    learned_theory, _ = mistle.learn(dl_measure=dl, minsup=support)
    learned_clauses = get_clauses(learned_theory)

    similarity = compare_theories(actual_theory, learned_clauses, alphabet_size)
    similarity_list.append(similarity)


print(similarity_list)

plt.figure(2)
plt.ylabel("Similarity")
plt.xlabel("Missingness Parameter")
plt.title(
    "Mistle is able to learn the actual theory when missingness paramter is varied"
)
plt.plot(missingness_parameters, similarity_list, marker="o")
plt.ylim(bottom=0.975, top=1)
mplcyberpunk.add_glow_effects()
# mplcyberpunk.make_lines_glow(plt)
# mplcyberpunk.add_underglow(plt)
plt.savefig("Experiments/exp1_plot2_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()


###############################################################################
# Plot 3: Alphabet Size
###############################################################################

alphabet_sizes = [4, 6, 8, 10, 12]
data_size = 400
missingness_parameter = 0.1
dl = "me"
support = 1
similarity_list = []

for alphabet_size in alphabet_sizes:
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)
    generator = TheoryNoisyGeneratorOnDataset(
        theory_object, data_size, missingness_parameter
    )
    positives, negatives = generator.generate_dataset()

    mistle = Mistle(positives, negatives)
    learned_theory, _ = mistle.learn(dl_measure=dl, minsup=support)
    learned_clauses = get_clauses(learned_theory)

    similarity = compare_theories(actual_theory, learned_clauses, alphabet_size)
    similarity_list.append(similarity)


print(similarity_list)

plt.figure(3)
plt.ylabel("Similarity")
plt.xlabel("Alphabet Size")
plt.title("Mistle is able to learn the actual theory when alphabet size is varied")
plt.plot(alphabet_sizes, similarity_list, marker="o")
plt.ylim(bottom=0.975, top=1)
mplcyberpunk.add_glow_effects()
# mplcyberpunk.make_lines_glow(plt)
# mplcyberpunk.add_underglow(plt)
plt.savefig("Experiments/exp1_plot3_v" + str(version) + ".png", bbox_inches="tight")
plt.show()
plt.close()
