from data_generators import TheoryNoisyGeneratorOnDataset, GeneratedTheory
import random
import numpy as np
from time import time
from mining4sat_wrapper import run_mining4sat
from mistle_v2 import get_dl, Mistle
import os
import matplotlib
import matplotlib.pyplot as plt
from statistics import mean, stdev

# plt.style.use("cyberpunk")
plt.style.use("seaborn")
matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=24)
matplotlib.rc("axes", titlesize=22)
matplotlib.rc("axes", labelsize=22)
matplotlib.rc("xtick", labelsize=22)
matplotlib.rc("ytick", labelsize=22)
matplotlib.rc("legend", fontsize=18)
matplotlib.rc("figure", titlesize=22)

num_iterations = 2
version = 3
seed = 0
random.seed(seed)
np.random.seed(seed)
os.chdir("..")
mining4sat_absolute_path = os.path.abspath("Resources/Mining4SAT")


def initialize_theory(negatives):
    theory = []
    for pa in negatives:
        theory.append(frozenset([-literal for literal in pa]))

    return theory


def get_alphabet_size(theory):
    alphabets = set()
    for clause in theory:
        for literal in clause:
            alphabets.add(abs(int(literal)))

    return len(alphabets)


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


###############################################################################
# Plot 1: Minsup Plot: Increase minimum support threshold (with LL)
###############################################################################


def plot1(minsup_list, alphabet_size=10, data_size=400, dl="ll", po_parameter=0.9):
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)
    generator = TheoryNoisyGeneratorOnDataset(
        theory_object, data_size, 1 - po_parameter
    )
    negatives = generator.generate_dataset(generate_only_negatives=True)

    theory = initialize_theory(negatives)
    initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
    print("Initial DL\t\t: " + str(initial_dl))

    mining4sat_compression_list = []
    mistle_compression_list = []
    mistle_prune_compression_list = []

    for m in minsup_list:
        if m < 1:
            support = max(int(m * data_size), 1)
        else:
            support = m

        mining4sat_theory = run_mining4sat(
            theory, support=support, code_path=mining4sat_absolute_path
        )
        if mining4sat_theory != []:
            mining4sat_dl = get_dl(
                dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )
        else:
            mining4sat_compression_list.append(0)

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            allow_empty_positives=True,
        )
        mistle_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=True,
            allow_empty_positives=True,
        )
        if mistle_theory is None:
            raise ("Empty Theory Learned")
        mistle_prune_compression_list.append(compression)

    return (
        mining4sat_compression_list,
        mistle_compression_list,
        mistle_prune_compression_list,
    )


minsup_list = [0.05, 0.1, 0.15, 0.2, 0.25, 0.3]
m4s_list = []
m_list = []
m_prune_list = []
for i in range(num_iterations):
    m4s, m, m_prune = plot1(minsup_list)
    m4s_list.append(m4s)
    m_list.append(m)
    m_prune_list.append(m_prune)

m4s_list = np.array(m4s_list)
m_list = np.array(m_list)
m_prune_list = np.array(m_prune_list)

mean_mining4sat_list = []
l_stddev_mining4sat_list = []
u_stddev_mining4sat_list = []
stddev_mining4sat_list = []

mean_mistle_list = []
l_stddev_mistle_list = []
u_stddev_mistle_list = []
stddev_mistle_list = []

mean_mistle_prune_list = []
l_stddev_mistle_prune_list = []
u_stddev_mistle_prune_list = []
stddev_mistle_prune_list = []

for i in range(len(minsup_list)):
    m = mean(m4s_list[:, i])
    std_dev = stdev(m4s_list[:, i])
    mean_mining4sat_list.append(m)
    stddev_mining4sat_list.append(std_dev)
    l_stddev_mining4sat_list.append(
        max(m - std_dev, -1)
    )  # Compression is a fraction between -1 to 1
    u_stddev_mining4sat_list.append(min(m + std_dev, 1))

    m = mean(m_list[:, i])
    std_dev = stdev(m_list[:, i])
    mean_mistle_list.append(m)
    stddev_mistle_list.append(std_dev)
    l_stddev_mistle_list.append(max(m - std_dev, -1))
    u_stddev_mistle_list.append(min(m + std_dev, 1))

    m = mean(m_prune_list[:, i])
    std_dev = stdev(m_prune_list[:, i])
    mean_mistle_prune_list.append(m)
    stddev_mistle_prune_list.append(std_dev)
    l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
    u_stddev_mistle_prune_list.append(min(m + std_dev, 1))

print("mean_mining4sat_list = " + str(mean_mining4sat_list))
print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))

print("mean_mistle_list = " + str(mean_mistle_list))
print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
print("stddev_mistle_list = " + str(stddev_mistle_list))

print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))


plt.figure(1)
plt.ylabel("Compression\n(Literal Length)")
plt.xlabel("Minimum support threshold")

plt.plot(minsup_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
plt.fill_between(
    minsup_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
)
plt.plot(minsup_list, mean_mistle_list, marker="o", label="Mistle (without pruning)")
plt.fill_between(minsup_list, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
plt.plot(minsup_list, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)")
plt.fill_between(
    minsup_list, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
)

# plt.legend(bbox_to_anchor=(0, 1.02), loc="upper left", ncol=3)
plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
plt.savefig("Experiments/exp3_plot1_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 2: Noise PLot: Increase partial observability paramter (with ME)
###############################################################################


def plot2(po_parameters, minsup=0, alphabet_size=10, data_size=400, dl="me"):
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)

    if minsup < 1:
        support = max(int(minsup * data_size), 1)
    else:
        support = minsup

    mining4sat_compression_list = []
    mistle_compression_list = []
    mistle_prune_compression_list = []

    for po_parameter in po_parameters:
        generator = TheoryNoisyGeneratorOnDataset(
            theory_object, data_size, 1 - po_parameter
        )
        negatives = generator.generate_dataset(generate_only_negatives=True)

        theory = initialize_theory(negatives)
        initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
        print("Initial DL\t\t: " + str(initial_dl))

        mining4sat_theory = run_mining4sat(
            theory, support=support, code_path=mining4sat_absolute_path
        )
        if mining4sat_theory != []:
            mining4sat_dl = get_dl(
                dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )
        else:
            mining4sat_compression_list.append(0)

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            allow_empty_positives=True,
        )
        mistle_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=True,
            allow_empty_positives=True,
        )
        if mistle_theory is None:
            raise ("Empty Theory Learned")
        mistle_prune_compression_list.append(compression)

    return (
        mining4sat_compression_list,
        mistle_compression_list,
        mistle_prune_compression_list,
    )


po_parameters = [0.5, 0.6, 0.7, 0.8, 0.9, 1]
m4s_list = []
m_list = []
m_prune_list = []
for i in range(num_iterations):
    m4s, m, m_prune = plot2(po_parameters)
    m4s_list.append(m4s)
    m_list.append(m)
    m_prune_list.append(m_prune)

m4s_list = np.array(m4s_list)
m_list = np.array(m_list)
m_prune_list = np.array(m_prune_list)

mean_mining4sat_list = []
l_stddev_mining4sat_list = []
u_stddev_mining4sat_list = []
stddev_mining4sat_list = []

mean_mistle_list = []
l_stddev_mistle_list = []
u_stddev_mistle_list = []
stddev_mistle_list = []

mean_mistle_prune_list = []
l_stddev_mistle_prune_list = []
u_stddev_mistle_prune_list = []
stddev_mistle_prune_list = []

for i in range(len(po_parameters)):
    m = mean(m4s_list[:, i])
    std_dev = stdev(m4s_list[:, i])
    mean_mining4sat_list.append(m)
    stddev_mining4sat_list.append(std_dev)
    l_stddev_mining4sat_list.append(
        max(m - std_dev, -1)
    )  # Compression is a fraction between -1 to 1
    u_stddev_mining4sat_list.append(min(m + std_dev, 1))

    m = mean(m_list[:, i])
    std_dev = stdev(m_list[:, i])
    mean_mistle_list.append(m)
    stddev_mistle_list.append(std_dev)
    l_stddev_mistle_list.append(max(m - std_dev, -1))
    u_stddev_mistle_list.append(min(m + std_dev, 1))

    m = mean(m_prune_list[:, i])
    std_dev = stdev(m_prune_list[:, i])
    mean_mistle_prune_list.append(m)
    stddev_mistle_prune_list.append(std_dev)
    l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
    u_stddev_mistle_prune_list.append(min(m + std_dev, 1))

print("mean_mining4sat_list = " + str(mean_mining4sat_list))
print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))

print("mean_mistle_list = " + str(mean_mistle_list))
print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
print("stddev_mistle_list = " + str(stddev_mistle_list))

print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))

# mean_mining4sat_list = [-0.5165008779899122, -0.5114854484296503, -0.4672979116215935, -0.44420168844832214, -0.28217336321539965, -0.16156035428103127]
# l_stddev_mining4sat_list = [-0.5497382180223043, -0.5155474522252742, -0.4887854770205793, -0.4604866562030725, -0.29054981590425366, -0.31186119119034195]
# u_stddev_mining4sat_list = [-0.48326353795752003, -0.5074234446340263, -0.44581034622260773, -0.4279167206935718, -0.27379691052654564, -0.011259517371720595]
# stddev_mining4sat_list = [0.03323734003239214, 0.00406200379562391, 0.02148756539898577, 0.01628496775475035, 0.008376452688854027, 0.15030083690931068]
# mean_mistle_list = [0.9846446648157683, 0.9745677438812004, 0.7215125533309197, 0.43879169669840973, 0.5219654134105021, 0.7026682723822678]
# l_stddev_mistle_list = [0.9748588551104048, 0.94238631722217, 0.49182572907385846, 0.4367308271153689, 0.49900453869442213, 0.6908403209497423]
# u_stddev_mistle_list = [0.9944304745211318, 1, 0.9511993775879809, 0.44085256628145053, 0.544926288126582, 0.7144962238147933]
# stddev_mistle_list = [0.009785809705363448, 0.03218142665903039, 0.22968682425706122, 0.0020608695830408004, 0.022960874716079913, 0.011827951432525479]
# mean_mistle_prune_list = [0.9911860337646479, 0.9827461945046763, 0.7475498377017451, 0.44563252127150677, 0.5313103754749928, 0.7026682723822678]
# l_stddev_mistle_prune_list = [0.9830924124438772, 0.9621308436365641, 0.511987231366251, 0.44474637189736016, 0.5050881426357009, 0.6908403209497423]
# u_stddev_mistle_prune_list = [0.9992796550854185, 1, 0.9831124440372392, 0.44651867064565337, 0.5575326083142847, 0.7144962238147933]
# stddev_mistle_prune_list = [0.00809362132077064, 0.020615350868112177, 0.23556260633549403, 0.0008861493741466283, 0.02622223283929193, 0.011827951432525479]

plt.figure(2)
plt.ylabel("Compression\n(Description Length)")
plt.xlabel("Partial Observability Parameter")

plt.plot(po_parameters, mean_mining4sat_list, marker="o", label="Mining4SAT")
plt.fill_between(
    po_parameters, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
)
plt.plot(po_parameters, mean_mistle_list, marker="o", label="Mistle (without pruning)")
plt.fill_between(po_parameters, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
plt.plot(
    po_parameters, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)"
)
plt.fill_between(
    po_parameters, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
)

plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
plt.savefig("Experiments/exp3_plot2_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()

##############################################################################
# Plot 3: Size Plot: Increase data size with DL = Modified Entropy
##############################################################################


def plot3(data_size_list, po_parameter=0.9, minsup=0, alphabet_size=10, dl="me"):
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)

    mining4sat_compression_list = []
    mistle_compression_list = []
    mistle_prune_compression_list = []

    for data_size in data_size_list:

        if minsup < 1:
            support = max(int(minsup * data_size), 1)
        else:
            support = minsup

        generator = TheoryNoisyGeneratorOnDataset(
            theory_object, data_size, 1 - po_parameter
        )
        negatives = generator.generate_dataset(generate_only_negatives=True)

        theory = initialize_theory(negatives)
        initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
        print("Initial DL\t\t: " + str(initial_dl))

        mining4sat_theory = run_mining4sat(
            theory, support=support, code_path=mining4sat_absolute_path
        )
        if mining4sat_theory != []:
            mining4sat_dl = get_dl(
                dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )
        else:
            mining4sat_compression_list.append(0)

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            allow_empty_positives=True,
        )
        mistle_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=True,
            allow_empty_positives=True,
        )
        if mistle_theory is None:
            raise ("Empty Theory Learned")
        mistle_prune_compression_list.append(compression)

    return (
        mining4sat_compression_list,
        mistle_compression_list,
        mistle_prune_compression_list,
    )


data_size_list = [100, 200, 300, 400, 500]
m4s_list = []
m_list = []
m_prune_list = []
for i in range(num_iterations):
    m4s, m, m_prune = plot3(data_size_list)
    m4s_list.append(m4s)
    m_list.append(m)
    m_prune_list.append(m_prune)

m4s_list = np.array(m4s_list)
m_list = np.array(m_list)
m_prune_list = np.array(m_prune_list)

mean_mining4sat_list = []
l_stddev_mining4sat_list = []
u_stddev_mining4sat_list = []
stddev_mining4sat_list = []

mean_mistle_list = []
l_stddev_mistle_list = []
u_stddev_mistle_list = []
stddev_mistle_list = []

mean_mistle_prune_list = []
l_stddev_mistle_prune_list = []
u_stddev_mistle_prune_list = []
stddev_mistle_prune_list = []

for i in range(len(data_size_list)):
    m = mean(m4s_list[:, i])
    std_dev = stdev(m4s_list[:, i])
    mean_mining4sat_list.append(m)
    stddev_mining4sat_list.append(std_dev)
    l_stddev_mining4sat_list.append(
        max(m - std_dev, -1)
    )  # Compression is a fraction between -1 to 1
    u_stddev_mining4sat_list.append(min(m + std_dev, 1))

    m = mean(m_list[:, i])
    std_dev = stdev(m_list[:, i])
    mean_mistle_list.append(m)
    stddev_mistle_list.append(std_dev)
    l_stddev_mistle_list.append(max(m - std_dev, -1))
    u_stddev_mistle_list.append(min(m + std_dev, 1))

    m = mean(m_prune_list[:, i])
    std_dev = stdev(m_prune_list[:, i])
    mean_mistle_prune_list.append(m)
    stddev_mistle_prune_list.append(std_dev)
    l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
    u_stddev_mistle_prune_list.append(min(m + std_dev, 1))

print("mean_mining4sat_list = " + str(mean_mining4sat_list))
print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))

print("mean_mistle_list = " + str(mean_mistle_list))
print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
print("stddev_mistle_list = " + str(stddev_mistle_list))

print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))

# mean_mining4sat_list = [-0.5220744762377628, -0.4351894015490604, -0.34367118562158594, -0.34018300667275375, -0.2851450280229149]
# l_stddev_mining4sat_list = [-0.5598192745246376, -0.4530737225311982, -0.34592914028952204, -0.40322413285804737, -0.2857518878881947]
# u_stddev_mining4sat_list = [-0.48432967795088805, -0.41730508056692256, -0.34141323095364984, -0.27714188048746013, -0.2845381681576351]
# stddev_mining4sat_list = [0.037744798286874784, 0.017884320982137835, 0.002257954667936093, 0.0630411261852936, 0.0006068598652797882]
# mean_mistle_list = [0.1467386134059837, 0.2968501489973946, 0.37866703681482594, 0.42764858325607635, 0.5132044298891045]
# l_stddev_mistle_list = [0.10287818880699781, 0.24514861391567133, 0.3308372166875355, 0.37882081272689383, 0.49157118296773045]
# u_stddev_mistle_list = [0.1905990380049696, 0.34855168407911785, 0.42649685694211636, 0.4764763537852589, 0.5348376768104787]
# stddev_mistle_list = [0.0438604245989859, 0.051701535081723274, 0.04782982012729044, 0.04882777052918255, 0.021633246921374104]
# mean_mistle_prune_list = [0.1467386134059837, 0.2968501489973946, 0.38202899325523754, 0.4357640773907223, 0.5255029074178552]
# l_stddev_mistle_prune_list = [0.10287818880699781, 0.24514861391567133, 0.3294446487338094, 0.3821938719420716, 0.5077263688379037]
# u_stddev_mistle_prune_list = [0.1905990380049696, 0.34855168407911785, 0.43461333777666566, 0.489334282839373, 0.5432794459978068]
# stddev_mistle_prune_list = [0.0438604245989859, 0.051701535081723274, 0.05258434452142812, 0.0535702054486507, 0.017776538579951574]

plt.figure(3)
plt.ylabel("Compression\n(Description Length)")
plt.xlabel("Data Size")

plt.plot(data_size_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
plt.fill_between(
    data_size_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
)
plt.plot(data_size_list, mean_mistle_list, marker="o", label="Mistle (without pruning)")
plt.fill_between(data_size_list, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
plt.plot(
    data_size_list, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)"
)
plt.fill_between(
    data_size_list, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
)

plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
plt.savefig("Experiments/exp3_plot3_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 4: Noise Plot: Increase noise with DL = Literal Length
###############################################################################


def plot4(po_parameters, minsup=0, alphabet_size=10, data_size=400, dl="ll"):
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)

    if minsup < 1:
        support = max(int(minsup * data_size), 1)
    else:
        support = minsup

    mining4sat_compression_list = []
    mistle_compression_list = []
    mistle_prune_compression_list = []

    for po_parameter in po_parameters:
        generator = TheoryNoisyGeneratorOnDataset(
            theory_object, data_size, 1 - po_parameter
        )
        negatives = generator.generate_dataset(generate_only_negatives=True)

        theory = initialize_theory(negatives)
        initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
        print("Initial DL\t\t: " + str(initial_dl))

        mining4sat_theory = run_mining4sat(
            theory, support=support, code_path=mining4sat_absolute_path
        )
        if mining4sat_theory != []:
            mining4sat_dl = get_dl(
                dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )
        else:
            mining4sat_compression_list.append(0)

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            allow_empty_positives=True,
        )
        mistle_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=True,
            allow_empty_positives=True,
        )
        if mistle_theory is None:
            raise ("Empty Theory Learned")
        mistle_prune_compression_list.append(compression)

    return (
        mining4sat_compression_list,
        mistle_compression_list,
        mistle_prune_compression_list,
    )


po_parameters = [0.5, 0.6, 0.7, 0.8, 0.9, 1]
m4s_list = []
m_list = []
m_prune_list = []
for i in range(num_iterations):
    m4s, m, m_prune = plot4(po_parameters)
    m4s_list.append(m4s)
    m_list.append(m)
    m_prune_list.append(m_prune)

m4s_list = np.array(m4s_list)
m_list = np.array(m_list)
m_prune_list = np.array(m_prune_list)

mean_mining4sat_list = []
l_stddev_mining4sat_list = []
u_stddev_mining4sat_list = []
stddev_mining4sat_list = []

mean_mistle_list = []
l_stddev_mistle_list = []
u_stddev_mistle_list = []
stddev_mistle_list = []

mean_mistle_prune_list = []
l_stddev_mistle_prune_list = []
u_stddev_mistle_prune_list = []
stddev_mistle_prune_list = []

for i in range(len(po_parameters)):
    m = mean(m4s_list[:, i])
    std_dev = stdev(m4s_list[:, i])
    mean_mining4sat_list.append(m)
    stddev_mining4sat_list.append(std_dev)
    l_stddev_mining4sat_list.append(
        max(m - std_dev, -1)
    )  # Compression is a fraction between -1 to 1
    u_stddev_mining4sat_list.append(min(m + std_dev, 1))

    m = mean(m_list[:, i])
    std_dev = stdev(m_list[:, i])
    mean_mistle_list.append(m)
    stddev_mistle_list.append(std_dev)
    l_stddev_mistle_list.append(max(m - std_dev, -1))
    u_stddev_mistle_list.append(min(m + std_dev, 1))

    m = mean(m_prune_list[:, i])
    std_dev = stdev(m_prune_list[:, i])
    mean_mistle_prune_list.append(m)
    stddev_mistle_prune_list.append(std_dev)
    l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
    u_stddev_mistle_prune_list.append(min(m + std_dev, 1))

print("mean_mining4sat_list = " + str(mean_mining4sat_list))
print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))

print("mean_mistle_list = " + str(mean_mistle_list))
print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
print("stddev_mistle_list = " + str(stddev_mistle_list))

print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))

plt.figure(4)
plt.ylabel("Compression\n(Literal Length)")
plt.xlabel("Partial Observability Parameter")

plt.plot(po_parameters, mean_mining4sat_list, marker="o", label="Mining4SAT")
plt.fill_between(
    po_parameters, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
)
plt.plot(po_parameters, mean_mistle_list, marker="o", label="Mistle (without pruning)")
plt.fill_between(po_parameters, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
plt.plot(
    po_parameters, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)"
)
plt.fill_between(
    po_parameters, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
)

plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
plt.savefig("Experiments/exp3_plot4_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 5: Size Plot: Increase data size with DL = Literal Length
###############################################################################


def plot5(data_size_list, po_parameter=0.9, minsup=0, alphabet_size=10, dl="ll"):
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)

    mining4sat_compression_list = []
    mistle_compression_list = []
    mistle_prune_compression_list = []

    for data_size in data_size_list:

        if minsup < 1:
            support = max(int(minsup * data_size), 1)
        else:
            support = minsup

        generator = TheoryNoisyGeneratorOnDataset(
            theory_object, data_size, 1 - po_parameter
        )
        negatives = generator.generate_dataset(generate_only_negatives=True)

        theory = initialize_theory(negatives)
        initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
        print("Initial DL\t\t: " + str(initial_dl))

        mining4sat_theory = run_mining4sat(
            theory, support=support, code_path=mining4sat_absolute_path
        )
        if mining4sat_theory != []:
            mining4sat_dl = get_dl(
                dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )
        else:
            mining4sat_compression_list.append(0)

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            allow_empty_positives=True,
        )
        mistle_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=True,
            allow_empty_positives=True,
        )
        if mistle_theory is None:
            raise ("Empty Theory Learned")
        mistle_prune_compression_list.append(compression)

    return (
        mining4sat_compression_list,
        mistle_compression_list,
        mistle_prune_compression_list,
    )


# data_size_list = [64, 128, 256, 512]
data_size_list = [100, 200, 300, 400, 500]
m4s_list = []
m_list = []
m_prune_list = []
for i in range(num_iterations):
    m4s, m, m_prune = plot5(data_size_list)
    m4s_list.append(m4s)
    m_list.append(m)
    m_prune_list.append(m_prune)

m4s_list = np.array(m4s_list)
m_list = np.array(m_list)
m_prune_list = np.array(m_prune_list)

mean_mining4sat_list = []
l_stddev_mining4sat_list = []
u_stddev_mining4sat_list = []
stddev_mining4sat_list = []

mean_mistle_list = []
l_stddev_mistle_list = []
u_stddev_mistle_list = []
stddev_mistle_list = []

mean_mistle_prune_list = []
l_stddev_mistle_prune_list = []
u_stddev_mistle_prune_list = []
stddev_mistle_prune_list = []

for i in range(len(data_size_list)):
    m = mean(m4s_list[:, i])
    std_dev = stdev(m4s_list[:, i])
    mean_mining4sat_list.append(m)
    stddev_mining4sat_list.append(std_dev)
    l_stddev_mining4sat_list.append(
        max(m - std_dev, -1)
    )  # Compression is a fraction between -1 to 1
    u_stddev_mining4sat_list.append(min(m + std_dev, 1))

    m = mean(m_list[:, i])
    std_dev = stdev(m_list[:, i])
    mean_mistle_list.append(m)
    stddev_mistle_list.append(std_dev)
    l_stddev_mistle_list.append(max(m - std_dev, -1))
    u_stddev_mistle_list.append(min(m + std_dev, 1))

    m = mean(m_prune_list[:, i])
    std_dev = stdev(m_prune_list[:, i])
    mean_mistle_prune_list.append(m)
    stddev_mistle_prune_list.append(std_dev)
    l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
    u_stddev_mistle_prune_list.append(min(m + std_dev, 1))

print("mean_mining4sat_list = " + str(mean_mining4sat_list))
print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))

print("mean_mistle_list = " + str(mean_mistle_list))
print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
print("stddev_mistle_list = " + str(stddev_mistle_list))

print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))

plt.figure(5)
plt.ylabel("Compression\n(Literal Length)")
plt.xlabel("Data Size")

plt.plot(data_size_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
plt.fill_between(
    data_size_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
)
plt.plot(data_size_list, mean_mistle_list, marker="o", label="Mistle (without pruning)")
plt.fill_between(data_size_list, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
plt.plot(
    data_size_list, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)"
)
plt.fill_between(
    data_size_list, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
)

plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
plt.savefig("Experiments/exp3_plot5_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()

# ###############################################################################
# # Plot 6: Size Plot: Compare Runtime
# ###############################################################################
#
# support = 1
# dl = "ll"
# size_list = [64, 128, 256, 512]
# negative_size_list = []
# mining4sat_runtime_list = []
# mistle_m1_runtime_list = []
# mistle_np_m1_runtime_list = []
# mistle_runtime_list = []
# mistle_np_runtime_list = []
#
# for size in size_list:
#     generator = TheoryNoisyGeneratorOnDataset(actual_theory, size, 0.1)
#     _, negatives = generator.generate_dataset()
#     negative_size_list.append(len(negatives))
#
#     theory = initialize_theory(negatives)
#     initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
#
#     # 1
#     start_time = time()
#     _ = run_mining4sat(theory, support=support, code_path=mining4sat_absolute_path)
#     mining4sat_runtime_list.append(time() - start_time)
#
#     # 2
#     start_time = time()
#     mistle = Mistle([], negatives)
#     _, _ = mistle.learn(
#         dl_measure=dl, minsup=support, lossy=False, prune=True, mining_steps=1
#     )
#     mistle_m1_runtime_list.append(time() - start_time)
#
#     # 3
#     start_time = time()
#     mistle = Mistle([], negatives)
#     _, _ = mistle.learn(
#         dl_measure=dl, minsup=support, lossy=False, prune=False, mining_steps=1
#     )
#     mistle_np_m1_runtime_list.append(time() - start_time)
#
#     # 4
#     start_time = time()
#     mistle = Mistle([], negatives)
#     _, _ = mistle.learn(
#         dl_measure=dl, minsup=support, lossy=False, prune=True, mining_steps=None
#     )
#     mistle_runtime_list.append(time() - start_time)
#
#     # 5
#     start_time = time()
#     mistle = Mistle([], negatives)
#     _, _ = mistle.learn(
#         dl_measure=dl, minsup=support, lossy=False, prune=False, mining_steps=None
#     )
#     mistle_np_runtime_list.append(time() - start_time)
#
#
# plt.figure(6)
# plt.ylabel("Runtime (in seconds)")
# plt.xlabel("Data size")
# # plt.title("Mining4SAT outperforms Mistle in runtime")
#
# plt.plot(negative_size_list, mining4sat_runtime_list, marker="o", label="Mining4SAT")
# plt.plot(
#     negative_size_list,
#     mistle_m1_runtime_list,
#     marker="o",
#     label="Mistle (pruning and mining only once)",
# )
# plt.plot(
#     negative_size_list,
#     mistle_np_m1_runtime_list,
#     marker="o",
#     label="Mistle (no pruning and mining only once)",
# )
# plt.plot(
#     negative_size_list,
#     mistle_np_runtime_list,
#     marker="o",
#     label="Mistle (no pruning and no mining cap)",
# )
# plt.plot(
#     negative_size_list,
#     mistle_runtime_list,
#     marker="o",
#     label="Mistle (pruning and no mining cap)",
# )
#
# plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.savefig("Experiments/exp3_plot6_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

###############################################################################
# Plot 7: Size plot: Use less operators and no pruning and mine once
###############################################################################


def plot7(data_size_list, po_parameter=0.9, minsup=0, alphabet_size=10, dl="ll"):
    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)

    mining4sat_compression_list = []
    mistle_w_compression_list = []
    mistle_w_s_compression_list = []
    mistle_all_compression_list = []

    for data_size in data_size_list:

        if minsup < 1:
            support = max(int(minsup * data_size), 1)
        else:
            support = minsup

        generator = TheoryNoisyGeneratorOnDataset(
            theory_object, data_size, 1 - po_parameter
        )
        negatives = generator.generate_dataset(generate_only_negatives=True)

        theory = initialize_theory(negatives)
        initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
        print("Initial DL\t\t: " + str(initial_dl))

        mining4sat_theory = run_mining4sat(
            theory, support=support, code_path=mining4sat_absolute_path
        )
        if mining4sat_theory != []:
            mining4sat_dl = get_dl(
                dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )
        else:
            mining4sat_compression_list.append(0)

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            mining_steps=1,
            permitted_operators={
                "D": False,
                "W": True,
                "V": False,
                "S": False,
                "R": False,
                "T": False,
            },
            allow_empty_positives=True,
        )
        mistle_w_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            mining_steps=1,
            permitted_operators={
                "D": False,
                "W": True,
                "V": False,
                "S": True,
                "R": False,
                "T": False,
            },
            allow_empty_positives=True,
        )
        mistle_w_s_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            mining_steps=1,
            allow_empty_positives=True,
        )
        mistle_all_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

    return (
        mining4sat_compression_list,
        mistle_w_compression_list,
        mistle_w_s_compression_list,
        mistle_all_compression_list,
    )


# data_size_list = [64, 128, 256, 512]
data_size_list = [100, 200, 300, 400, 500]
m4s_list = []
m1_list = []
m2_list = []
m3_list = []
for i in range(num_iterations):
    m4s, m1, m2, m3 = plot7(data_size_list)
    m4s_list.append(m4s)
    m1_list.append(m1)
    m2_list.append(m2)
    m3_list.append(m3)

m4s_list = np.array(m4s_list)
m1_list = np.array(m1_list)
m2_list = np.array(m2_list)
m3_list = np.array(m3_list)

mean_mining4sat_list = []
l_stddev_mining4sat_list = []
u_stddev_mining4sat_list = []
stddev_mining4sat_list = []

mean_mistle1_list = []
l_stddev_mistle1_list = []
u_stddev_mistle1_list = []
stddev_mistle1_list = []

mean_mistle2_list = []
l_stddev_mistle2_list = []
u_stddev_mistle2_list = []
stddev_mistle2_list = []

mean_mistle3_list = []
l_stddev_mistle3_list = []
u_stddev_mistle3_list = []
stddev_mistle3_list = []

for i in range(len(data_size_list)):
    m = mean(m4s_list[:, i])
    std_dev = stdev(m4s_list[:, i])
    mean_mining4sat_list.append(m)
    stddev_mining4sat_list.append(std_dev)
    l_stddev_mining4sat_list.append(
        max(m - std_dev, -1)
    )  # Compression is a fraction between -1 to 1
    u_stddev_mining4sat_list.append(min(m + std_dev, 1))

    m = mean(m1_list[:, i])
    std_dev = stdev(m1_list[:, i])
    mean_mistle1_list.append(m)
    stddev_mistle1_list.append(std_dev)
    l_stddev_mistle1_list.append(max(m - std_dev, -1))
    u_stddev_mistle1_list.append(min(m + std_dev, 1))

    m = mean(m2_list[:, i])
    std_dev = stdev(m2_list[:, i])
    mean_mistle2_list.append(m)
    stddev_mistle2_list.append(std_dev)
    l_stddev_mistle2_list.append(max(m - std_dev, -1))
    u_stddev_mistle2_list.append(min(m + std_dev, 1))

    m = mean(m3_list[:, i])
    std_dev = stdev(m3_list[:, i])
    mean_mistle3_list.append(m)
    stddev_mistle3_list.append(std_dev)
    l_stddev_mistle3_list.append(max(m - std_dev, -1))
    u_stddev_mistle3_list.append(min(m + std_dev, 1))

print("mean_mining4sat_list = " + str(mean_mining4sat_list))
print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))

print("mean_mistle1_list = " + str(mean_mistle1_list))
print("l_stddev_mistle1_list = " + str(l_stddev_mistle1_list))
print("u_stddev_mistle1_list = " + str(u_stddev_mistle1_list))
print("stddev_mistle1_list = " + str(stddev_mistle1_list))

print("mean_mistle2_list = " + str(mean_mistle2_list))
print("l_stddev_mistle2_list = " + str(l_stddev_mistle2_list))
print("u_stddev_mistle2_list = " + str(u_stddev_mistle2_list))
print("stddev_mistle2_list = " + str(stddev_mistle2_list))

print("mean_mistle3_list = " + str(mean_mistle3_list))
print("l_stddev_mistle3_list = " + str(l_stddev_mistle3_list))
print("u_stddev_mistle3_list = " + str(u_stddev_mistle3_list))
print("stddev_mistle3_list = " + str(stddev_mistle3_list))


plt.figure(7)
plt.ylabel("Compression\n(Literal Length)")
plt.xlabel("Data Size")

plt.plot(data_size_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
plt.fill_between(
    data_size_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
)
plt.plot(data_size_list, mean_mistle1_list, marker="o", label="Mistle - W operator")
plt.fill_between(
    data_size_list, l_stddev_mistle1_list, u_stddev_mistle1_list, alpha=0.3
)
plt.plot(data_size_list, mean_mistle2_list, marker="o", label="Mistle - W, S operators")
plt.fill_between(
    data_size_list, l_stddev_mistle2_list, u_stddev_mistle2_list, alpha=0.3
)
plt.plot(
    data_size_list,
    mean_mistle3_list,
    marker="o",
    label="Mistle - all lossless operators",
)
plt.fill_between(
    data_size_list, l_stddev_mistle3_list, u_stddev_mistle3_list, alpha=0.3
)

plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
plt.savefig("Experiments/exp3_plot7_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()

###############################################################################
# Plot 8: Noise plot: Use less operators and no pruning and mine once
###############################################################################


def plot8(po_parameters, data_size=400, minsup=0, alphabet_size=10, dl="ll"):

    actual_theory = generate_theory(alphabet_size)
    theory_object = GeneratedTheory(actual_theory)

    mining4sat_compression_list = []
    mistle_w_compression_list = []
    mistle_w_s_compression_list = []
    mistle_all_compression_list = []

    for po_parameter in po_parameters:

        if minsup < 1:
            support = max(int(minsup * data_size), 1)
        else:
            support = minsup

        generator = TheoryNoisyGeneratorOnDataset(
            theory_object, data_size, 1 - po_parameter
        )
        negatives = generator.generate_dataset(generate_only_negatives=True)

        theory = initialize_theory(negatives)
        initial_dl = get_dl(dl, theory, [], get_alphabet_size(theory))
        print("Initial DL\t\t: " + str(initial_dl))

        mining4sat_theory = run_mining4sat(
            theory, support=support, code_path=mining4sat_absolute_path
        )
        if mining4sat_theory != []:
            mining4sat_dl = get_dl(
                dl, mining4sat_theory, [], get_alphabet_size(mining4sat_theory)
            )
            mining4sat_compression_list.append(
                (initial_dl - mining4sat_dl) / initial_dl
            )
        else:
            mining4sat_compression_list.append(0)

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            mining_steps=1,
            permitted_operators={
                "D": False,
                "W": True,
                "V": False,
                "S": False,
                "R": False,
                "T": False,
            },
            allow_empty_positives=True,
        )
        mistle_w_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            mining_steps=1,
            permitted_operators={
                "D": False,
                "W": True,
                "V": False,
                "S": True,
                "R": False,
                "T": False,
            },
            allow_empty_positives=True,
        )
        mistle_w_s_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl,
            minsup=support,
            lossy=False,
            prune=False,
            mining_steps=1,
            allow_empty_positives=True,
        )
        mistle_all_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

    return (
        mining4sat_compression_list,
        mistle_w_compression_list,
        mistle_w_s_compression_list,
        mistle_all_compression_list,
    )


po_parameters = [0.5, 0.6, 0.7, 0.8, 0.9, 1]
m4s_list = []
m1_list = []
m2_list = []
m3_list = []
for i in range(num_iterations):
    m4s, m1, m2, m3 = plot8(po_parameters)
    m4s_list.append(m4s)
    m1_list.append(m1)
    m2_list.append(m2)
    m3_list.append(m3)

m4s_list = np.array(m4s_list)
m1_list = np.array(m1_list)
m2_list = np.array(m2_list)
m3_list = np.array(m3_list)

mean_mining4sat_list = []
l_stddev_mining4sat_list = []
u_stddev_mining4sat_list = []
stddev_mining4sat_list = []

mean_mistle1_list = []
l_stddev_mistle1_list = []
u_stddev_mistle1_list = []
stddev_mistle1_list = []

mean_mistle2_list = []
l_stddev_mistle2_list = []
u_stddev_mistle2_list = []
stddev_mistle2_list = []

mean_mistle3_list = []
l_stddev_mistle3_list = []
u_stddev_mistle3_list = []
stddev_mistle3_list = []

for i in range(len(data_size_list)):
    m = mean(m4s_list[:, i])
    std_dev = stdev(m4s_list[:, i])
    mean_mining4sat_list.append(m)
    stddev_mining4sat_list.append(std_dev)
    l_stddev_mining4sat_list.append(
        max(m - std_dev, -1)
    )  # Compression is a fraction between -1 to 1
    u_stddev_mining4sat_list.append(min(m + std_dev, 1))

    m = mean(m1_list[:, i])
    std_dev = stdev(m1_list[:, i])
    mean_mistle1_list.append(m)
    stddev_mistle1_list.append(std_dev)
    l_stddev_mistle1_list.append(max(m - std_dev, -1))
    u_stddev_mistle1_list.append(min(m + std_dev, 1))

    m = mean(m2_list[:, i])
    std_dev = stdev(m2_list[:, i])
    mean_mistle2_list.append(m)
    stddev_mistle2_list.append(std_dev)
    l_stddev_mistle2_list.append(max(m - std_dev, -1))
    u_stddev_mistle2_list.append(min(m + std_dev, 1))

    m = mean(m3_list[:, i])
    std_dev = stdev(m3_list[:, i])
    mean_mistle3_list.append(m)
    stddev_mistle3_list.append(std_dev)
    l_stddev_mistle3_list.append(max(m - std_dev, -1))
    u_stddev_mistle3_list.append(min(m + std_dev, 1))

print("mean_mining4sat_list = " + str(mean_mining4sat_list))
print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))

print("mean_mistle1_list = " + str(mean_mistle1_list))
print("l_stddev_mistle1_list = " + str(l_stddev_mistle1_list))
print("u_stddev_mistle1_list = " + str(u_stddev_mistle1_list))
print("stddev_mistle1_list = " + str(stddev_mistle1_list))

print("mean_mistle2_list = " + str(mean_mistle2_list))
print("l_stddev_mistle2_list = " + str(l_stddev_mistle2_list))
print("u_stddev_mistle2_list = " + str(u_stddev_mistle2_list))
print("stddev_mistle2_list = " + str(stddev_mistle2_list))

print("mean_mistle3_list = " + str(mean_mistle3_list))
print("l_stddev_mistle3_list = " + str(l_stddev_mistle3_list))
print("u_stddev_mistle3_list = " + str(u_stddev_mistle3_list))
print("stddev_mistle3_list = " + str(stddev_mistle3_list))

plt.figure(8)
plt.ylabel("Compression\n(Literal Length)")
plt.xlabel("Data Size")

plt.plot(data_size_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
plt.fill_between(
    data_size_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
)
plt.plot(data_size_list, mean_mistle1_list, marker="o", label="Mistle - W operator")
plt.fill_between(
    data_size_list, l_stddev_mistle1_list, u_stddev_mistle1_list, alpha=0.3
)
plt.plot(data_size_list, mean_mistle2_list, marker="o", label="Mistle - W, S operators")
plt.fill_between(
    data_size_list, l_stddev_mistle2_list, u_stddev_mistle2_list, alpha=0.3
)
plt.plot(
    data_size_list,
    mean_mistle3_list,
    marker="o",
    label="Mistle - all lossless operators",
)
plt.fill_between(
    data_size_list, l_stddev_mistle3_list, u_stddev_mistle3_list, alpha=0.3
)

plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
plt.savefig("Experiments/exp3_plot8_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()
