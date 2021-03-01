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

num_iterations = 10
version = 5
seed = 0
random.seed(seed)
np.random.seed(seed)
# os.chdir("..")
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


###############################################################################
# Plot 1: Minsup Plot: Increase minimum support threshold (with LL)
###############################################################################


def plot1(minsup_list, alphabet_size=14, data_size=400, dl="ll", po_parameter=0.9):
    actual_theory = generate_complex_theory(alphabet_size)
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
            minsup=m,
            lossy=False,
            prune=False,
            allow_empty_positives=True,
        )
        mistle_compression_list.append(compression)
        if mistle_theory is None:
            raise ("Empty Theory Learned")

        mistle = Mistle([], negatives)
        mistle_theory, compression = mistle.learn(
            dl_measure=dl, minsup=m, lossy=False, prune=True, allow_empty_positives=True
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

# # version 4; num_iterations 10
# # mean_mining4sat_list = [0.5308517044630887, 0.4759440853010015, 0.4217773666562703, 0.3584949553384384, 0.19162045415675605, 0.10470237599836019]
# # l_stddev_mining4sat_list = [0.47220980076381663, 0.4144441885517034, 0.34788779407969517, 0.3001622560098504, 0.09602451364613619, 0.007897513610471196]
# # u_stddev_mining4sat_list = [0.5894936081623607, 0.5374439820502996, 0.49566693923284544, 0.41682765466702637, 0.2872163946673759, 0.20150723838624918]
# # stddev_mining4sat_list = [0.05864190369927206, 0.06149989674929812, 0.07388957257657516, 0.05833269932858799, 0.09559594051061986, 0.09680486238788899]
# # mean_mistle_list = [0.5613033254066407, 0.5322241170171031, 0.5046815629320083, 0.40898118169886544, 0.27307284390379, 0.21860527351103137]
# # l_stddev_mistle_list = [0.4115611946278379, 0.3722644256263424, 0.3386085024558574, 0.20971637049993458, 0.018476732791587402, -0.055448281029083335]
# # u_stddev_mistle_list = [0.7110454561854436, 0.692183808407864, 0.6707546234081592, 0.6082459928977964, 0.5276689550159925, 0.4926588280511461]
# # stddev_mistle_list = [0.1497421307788028, 0.15995969139076077, 0.16607306047615086, 0.19926481119893086, 0.2545961111122026, 0.2740535545401147]
# # mean_mistle_prune_list = [0.746942989841752, 0.7332038669186653, 0.7234773452602723, 0.683848018055645, 0.5832993529506035, 0.4599106878781747]
# # l_stddev_mistle_prune_list = [0.6431614722294856, 0.625952585841419, 0.6120191341041247, 0.5587273091413096, 0.37513904955734706, 0.13357037486110696]
# # u_stddev_mistle_prune_list = [0.8507245074540184, 0.8404551479959117, 0.8349355564164199, 0.8089687269699803, 0.7914596563438601, 0.7862510008952424]
# # stddev_mistle_prune_list = [0.10378151761226635, 0.10725128107724632, 0.11145821115614767, 0.1251207089143353, 0.20816030339325647, 0.32634031301706773]
#
# plt.figure(1)
# plt.ylabel("Compression\n(Literal Length)")
# plt.xlabel("Minimum support threshold")
#
# plt.plot(minsup_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
# plt.fill_between(
#     minsup_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
# )
# plt.plot(minsup_list, mean_mistle_list, marker="o", label="Mistle (without pruning)")
# plt.fill_between(minsup_list, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
# plt.plot(minsup_list, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)")
# plt.fill_between(
#     minsup_list, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
# )
#
# # plt.legend(bbox_to_anchor=(0, 1.02), loc="upper left", ncol=3)
# plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.savefig("Experiments/exp3_plot1_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

###############################################################################
# Plot 2: Noise PLot: Increase partial observability parameter (with ME)
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


# po_parameters = [0.5, 0.6, 0.7, 0.8, 0.9, 1]
# m4s_list = []
# m_list = []
# m_prune_list = []
# for i in range(num_iterations):
#     m4s, m, m_prune = plot2(po_parameters)
#     m4s_list.append(m4s)
#     m_list.append(m)
#     m_prune_list.append(m_prune)
#
# m4s_list = np.array(m4s_list)
# m_list = np.array(m_list)
# m_prune_list = np.array(m_prune_list)
#
# mean_mining4sat_list = []
# l_stddev_mining4sat_list = []
# u_stddev_mining4sat_list = []
# stddev_mining4sat_list = []
#
# mean_mistle_list = []
# l_stddev_mistle_list = []
# u_stddev_mistle_list = []
# stddev_mistle_list = []
#
# mean_mistle_prune_list = []
# l_stddev_mistle_prune_list = []
# u_stddev_mistle_prune_list = []
# stddev_mistle_prune_list = []
#
# for i in range(len(po_parameters)):
#     m = mean(m4s_list[:, i])
#     std_dev = stdev(m4s_list[:, i])
#     mean_mining4sat_list.append(m)
#     stddev_mining4sat_list.append(std_dev)
#     l_stddev_mining4sat_list.append(
#         max(m - std_dev, -1)
#     )  # Compression is a fraction between -1 to 1
#     u_stddev_mining4sat_list.append(min(m + std_dev, 1))
#
#     m = mean(m_list[:, i])
#     std_dev = stdev(m_list[:, i])
#     mean_mistle_list.append(m)
#     stddev_mistle_list.append(std_dev)
#     l_stddev_mistle_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_list.append(min(m + std_dev, 1))
#
#     m = mean(m_prune_list[:, i])
#     std_dev = stdev(m_prune_list[:, i])
#     mean_mistle_prune_list.append(m)
#     stddev_mistle_prune_list.append(std_dev)
#     l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_prune_list.append(min(m + std_dev, 1))
#
# print("mean_mining4sat_list = " + str(mean_mining4sat_list))
# print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
# print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
# print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))
#
# print("mean_mistle_list = " + str(mean_mistle_list))
# print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
# print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
# print("stddev_mistle_list = " + str(stddev_mistle_list))
#
# print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
# print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
# print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
# print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))
#
# #v4 10 iterations
# mean_mining4sat_list = [-0.5298565082632883, -0.5118550632153228, -0.4697861555449255, -0.4389199655710129, -0.33740177087726003, -0.23564053696889958]
# l_stddev_mining4sat_list = [-0.5579118910733923, -0.547907304935758, -0.5102847315316088, -0.45349344437674916, -0.38901606761191415, -0.36721224613758546]
# u_stddev_mining4sat_list = [-0.5018011254531843, -0.4758028214948877, -0.42928757955824215, -0.4243464867652767, -0.2857874741426059, -0.1040688278002137]
# stddev_mining4sat_list = [0.028055382810104047, 0.03605224172043515, 0.0404985759866833, 0.014573478805736237, 0.05161429673465411, 0.13157170916868588]
# mean_mistle_list = [0.9541624858519193, 0.8972146738157218, 0.6461470877048132, 0.4397228732977092, 0.46344244130321044, 0.6622834716988386]
# l_stddev_mistle_list = [0.9189177387790208, 0.8208437034439807, 0.4559329328316744, 0.38632146283121443, 0.4057790515285803, 0.6198382373172007]
# u_stddev_mistle_list = [0.9894072329248178, 0.973585644187463, 0.8363612425779521, 0.493124283764204, 0.5211058310778406, 0.7047287060804766]
# stddev_mistle_list = [0.03524474707289851, 0.07637097037174109, 0.1902141548731388, 0.053401410466494784, 0.05766338977463013, 0.042445234381637935]
# mean_mistle_prune_list = [0.9662619084281171, 0.9215865093748798, 0.6630370733320944, 0.45388203795138554, 0.47336406760648403, 0.6622834716988386]
# l_stddev_mistle_prune_list = [0.9336839378703597, 0.8405843662468797, 0.4633299894372852, 0.39884323579717473, 0.4149230409225281, 0.6198382373172007]
# u_stddev_mistle_prune_list = [0.9988398789858745, 1, 0.8627441572269036, 0.5089208401055964, 0.53180509429044, 0.7047287060804766]
# stddev_mistle_prune_list = [0.03257797055775736, 0.08100214312800016, 0.1997070838948092, 0.05503880215421082, 0.05844102668395595, 0.042445234381637935]
#
# plt.figure(2)
# plt.ylabel("Compression\n(Description Length)")
# plt.xlabel("Partial Observability Parameter")
#
# plt.plot(po_parameters, mean_mining4sat_list, marker="o", label="Mining4SAT")
# plt.fill_between(
#     po_parameters, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
# )
# # plt.plot(po_parameters, mean_mistle_list, marker="o", label="Mistle (without pruning)")
# # plt.fill_between(po_parameters, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
# plt.plot(po_parameters, mean_mistle_prune_list, marker="o", label="Mistle")
# plt.fill_between(
#     po_parameters, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
# )
#
# # plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.legend(bbox_to_anchor=(0, 1.02), loc="upper left", ncol=2)
# plt.savefig("Experiments/exp3_plot2_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

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


# data_size_list = [100, 200, 300, 400, 500]
# m4s_list = []
# m_list = []
# m_prune_list = []
# for i in range(num_iterations):
#     m4s, m, m_prune = plot3(data_size_list)
#     m4s_list.append(m4s)
#     m_list.append(m)
#     m_prune_list.append(m_prune)
#
# m4s_list = np.array(m4s_list)
# m_list = np.array(m_list)
# m_prune_list = np.array(m_prune_list)
#
# mean_mining4sat_list = []
# l_stddev_mining4sat_list = []
# u_stddev_mining4sat_list = []
# stddev_mining4sat_list = []
#
# mean_mistle_list = []
# l_stddev_mistle_list = []
# u_stddev_mistle_list = []
# stddev_mistle_list = []
#
# mean_mistle_prune_list = []
# l_stddev_mistle_prune_list = []
# u_stddev_mistle_prune_list = []
# stddev_mistle_prune_list = []
#
# for i in range(len(data_size_list)):
#     m = mean(m4s_list[:, i])
#     std_dev = stdev(m4s_list[:, i])
#     mean_mining4sat_list.append(m)
#     stddev_mining4sat_list.append(std_dev)
#     l_stddev_mining4sat_list.append(
#         max(m - std_dev, -1)
#     )  # Compression is a fraction between -1 to 1
#     u_stddev_mining4sat_list.append(min(m + std_dev, 1))
#
#     m = mean(m_list[:, i])
#     std_dev = stdev(m_list[:, i])
#     mean_mistle_list.append(m)
#     stddev_mistle_list.append(std_dev)
#     l_stddev_mistle_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_list.append(min(m + std_dev, 1))
#
#     m = mean(m_prune_list[:, i])
#     std_dev = stdev(m_prune_list[:, i])
#     mean_mistle_prune_list.append(m)
#     stddev_mistle_prune_list.append(std_dev)
#     l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_prune_list.append(min(m + std_dev, 1))
#
# print("mean_mining4sat_list = " + str(mean_mining4sat_list))
# print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
# print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
# print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))
#
# print("mean_mistle_list = " + str(mean_mistle_list))
# print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
# print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
# print("stddev_mistle_list = " + str(stddev_mistle_list))
#
# print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
# print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
# print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
# print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))
#
# # version4; 10 iterations
# # mean_mining4sat_list = [-0.4835820099757269, -0.41280139654238446, -0.3468084463252358, -0.31451521761868534, -0.29400449859595135]
# # l_stddev_mining4sat_list = [-0.5871394903045929, -0.5216923973712443, -0.4550622020555044, -0.42908885679719244, -0.40706657685902786]
# # u_stddev_mining4sat_list = [-0.380024529646861, -0.3039103957135247, -0.23855469059496717, -0.19994157844017824, -0.18094242033287483]
# # stddev_mining4sat_list = [0.10355748032886593, 0.10889100082885979, 0.1082537557302686, 0.11457363917850712, 0.11306207826307653]
# # mean_mistle_list = [0.2607879735692339, 0.4587835017631927, 0.541166930088553, 0.6006019191604719, 0.6570628540647839]
# # l_stddev_mistle_list = [0.12950980390197706, 0.2385335484333576, 0.33609865238973047, 0.3910726452656219, 0.4747729715549673]
# # u_stddev_mistle_list = [0.3920661432364907, 0.6790334550930278, 0.7462352077873755, 0.8101311930553219, 0.8393527365746005]
# # stddev_mistle_list = [0.13127816966725683, 0.2202499533298351, 0.20506827769882252, 0.20952927389485002, 0.18228988250981665]
# # mean_mistle_prune_list = [0.2845748157123364, 0.4617023991991724, 0.5448479600009841, 0.6087231695599177, 0.6672941099842363]
# # l_stddev_mistle_prune_list = [0.10636774598577378, 0.24209836018200176, 0.3391885030644534, 0.40035403873703157, 0.4863697079124233]
# # u_stddev_mistle_prune_list = [0.462781885438899, 0.681306438216343, 0.7505074169375148, 0.8170923003828038, 0.8482185120560494]
# # stddev_mistle_prune_list = [0.1782070697265626, 0.21960403901717063, 0.2056594569365307, 0.20836913082288608, 0.180924402071813]
#
# plt.figure(3)
# plt.ylabel("Compression\n(Description Length)")
# plt.xlabel("Data Size")
#
# plt.plot(data_size_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
# plt.fill_between(
#     data_size_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
# )
# plt.plot(data_size_list, mean_mistle_list, marker="o", label="Mistle (without pruning)")
# plt.fill_between(data_size_list, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
# plt.plot(
#     data_size_list, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)"
# )
# plt.fill_between(
#     data_size_list, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
# )
#
# plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.savefig("Experiments/exp3_plot3_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

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


# po_parameters = [0.5, 0.6, 0.7, 0.8, 0.9, 1]
# m4s_list = []
# m_list = []
# m_prune_list = []
# for i in range(num_iterations):
#     m4s, m, m_prune = plot4(po_parameters)
#     m4s_list.append(m4s)
#     m_list.append(m)
#     m_prune_list.append(m_prune)
#
# m4s_list = np.array(m4s_list)
# m_list = np.array(m_list)
# m_prune_list = np.array(m_prune_list)
#
# mean_mining4sat_list = []
# l_stddev_mining4sat_list = []
# u_stddev_mining4sat_list = []
# stddev_mining4sat_list = []
#
# mean_mistle_list = []
# l_stddev_mistle_list = []
# u_stddev_mistle_list = []
# stddev_mistle_list = []
#
# mean_mistle_prune_list = []
# l_stddev_mistle_prune_list = []
# u_stddev_mistle_prune_list = []
# stddev_mistle_prune_list = []
#
# for i in range(len(po_parameters)):
#     m = mean(m4s_list[:, i])
#     std_dev = stdev(m4s_list[:, i])
#     mean_mining4sat_list.append(m)
#     stddev_mining4sat_list.append(std_dev)
#     l_stddev_mining4sat_list.append(
#         max(m - std_dev, -1)
#     )  # Compression is a fraction between -1 to 1
#     u_stddev_mining4sat_list.append(min(m + std_dev, 1))
#
#     m = mean(m_list[:, i])
#     std_dev = stdev(m_list[:, i])
#     mean_mistle_list.append(m)
#     stddev_mistle_list.append(std_dev)
#     l_stddev_mistle_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_list.append(min(m + std_dev, 1))
#
#     m = mean(m_prune_list[:, i])
#     std_dev = stdev(m_prune_list[:, i])
#     mean_mistle_prune_list.append(m)
#     stddev_mistle_prune_list.append(std_dev)
#     l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_prune_list.append(min(m + std_dev, 1))
#
# print("mean_mining4sat_list = " + str(mean_mining4sat_list))
# print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
# print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
# print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))
#
# print("mean_mistle_list = " + str(mean_mistle_list))
# print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
# print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
# print("stddev_mistle_list = " + str(stddev_mistle_list))
#
# print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
# print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
# print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
# print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))
#
#
# # version 4, num_iterations 10
# # mean_mining4sat_list = [0.36167005006254194, 0.40379115233541973, 0.4518806845933667, 0.49461657143786464, 0.5634443854200426, 0.6457999999999999]
# # l_stddev_mining4sat_list = [0.3433369129300138, 0.385371911728452, 0.4360789210588824, 0.48632385046945686, 0.5470194282632608, 0.6101779874546962]
# # u_stddev_mining4sat_list = [0.3800031871950701, 0.4222103929423875, 0.46768244812785104, 0.5029092924062724, 0.5798693425768244, 0.6814220125453037]
# # stddev_mining4sat_list = [0.018333137132528182, 0.01841924060696776, 0.015801763534484328, 0.008292720968407774, 0.016424957156781862, 0.03562201254530376]
# # mean_mistle_list = [0.8346818236198736, 0.597390025366827, 0.5029121262975371, 0.5054118187431345, 0.5759738878551149, 0.719575]
# # l_stddev_mistle_list = [0.7378247908483149, 0.49626203347980746, 0.4326470884245695, 0.4896849051892896, 0.5554313900456025, 0.7063310024077991]
# # u_stddev_mistle_list = [0.9315388563914324, 0.6985180172538464, 0.5731771641705048, 0.5211387322969793, 0.5965163856646274, 0.7328189975922008]
# # stddev_mistle_list = [0.09685703277155877, 0.10112799188701949, 0.07026503787296762, 0.015726913553844857, 0.020542497809512422, 0.013243997592200876]
# # mean_mistle_prune_list = [0.9760345463836486, 0.9269325576325638, 0.8509223550207345, 0.7611211588426247, 0.7125971711207684, 0.719575]
# # l_stddev_mistle_prune_list = [0.9584875936947781, 0.8967162269003766, 0.8016757667279598, 0.7253983294945825, 0.683946466649498, 0.7063310024077991]
# # u_stddev_mistle_prune_list = [0.9935814990725191, 0.957148888364751, 0.9001689433135093, 0.7968439881906668, 0.7412478755920388, 0.7328189975922008]
# # stddev_mistle_prune_list = [0.01754695268887054, 0.03021633073218717, 0.049246588292774736, 0.03572282934804222, 0.028650704471270425, 0.013243997592200876]
#
# plt.figure(4)
# plt.ylabel("Compression\n(Literal Length)")
# plt.xlabel("Partial Observability Parameter")
#
# plt.plot(po_parameters, mean_mining4sat_list, marker="o", label="Mining4SAT")
# plt.fill_between(
#     po_parameters, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
# )
# plt.plot(po_parameters, mean_mistle_list, marker="o", label="Mistle (without pruning)")
# plt.fill_between(po_parameters, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
# plt.plot(
#     po_parameters, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)"
# )
# plt.fill_between(
#     po_parameters, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
# )
#
# plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.savefig("Experiments/exp3_plot4_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

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


# data_size_list = [100, 200, 300, 400, 500]
# m4s_list = []
# m_list = []
# m_prune_list = []
# for i in range(num_iterations):
#     m4s, m, m_prune = plot5(data_size_list)
#     m4s_list.append(m4s)
#     m_list.append(m)
#     m_prune_list.append(m_prune)
#
# m4s_list = np.array(m4s_list)
# m_list = np.array(m_list)
# m_prune_list = np.array(m_prune_list)
#
# mean_mining4sat_list = []
# l_stddev_mining4sat_list = []
# u_stddev_mining4sat_list = []
# stddev_mining4sat_list = []
#
# mean_mistle_list = []
# l_stddev_mistle_list = []
# u_stddev_mistle_list = []
# stddev_mistle_list = []
#
# mean_mistle_prune_list = []
# l_stddev_mistle_prune_list = []
# u_stddev_mistle_prune_list = []
# stddev_mistle_prune_list = []
#
# for i in range(len(data_size_list)):
#     m = mean(m4s_list[:, i])
#     std_dev = stdev(m4s_list[:, i])
#     mean_mining4sat_list.append(m)
#     stddev_mining4sat_list.append(std_dev)
#     l_stddev_mining4sat_list.append(
#         max(m - std_dev, -1)
#     )  # Compression is a fraction between -1 to 1
#     u_stddev_mining4sat_list.append(min(m + std_dev, 1))
#
#     m = mean(m_list[:, i])
#     std_dev = stdev(m_list[:, i])
#     mean_mistle_list.append(m)
#     stddev_mistle_list.append(std_dev)
#     l_stddev_mistle_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_list.append(min(m + std_dev, 1))
#
#     m = mean(m_prune_list[:, i])
#     std_dev = stdev(m_prune_list[:, i])
#     mean_mistle_prune_list.append(m)
#     stddev_mistle_prune_list.append(std_dev)
#     l_stddev_mistle_prune_list.append(max(m - std_dev, -1))
#     u_stddev_mistle_prune_list.append(min(m + std_dev, 1))
#
# print("mean_mining4sat_list = " + str(mean_mining4sat_list))
# print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
# print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
# print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))
#
# print("mean_mistle_list = " + str(mean_mistle_list))
# print("l_stddev_mistle_list = " + str(l_stddev_mistle_list))
# print("u_stddev_mistle_list = " + str(u_stddev_mistle_list))
# print("stddev_mistle_list = " + str(stddev_mistle_list))
#
# print("mean_mistle_prune_list = " + str(mean_mistle_prune_list))
# print("l_stddev_mistle_prune_list = " + str(l_stddev_mistle_prune_list))
# print("u_stddev_mistle_prune_list = " + str(u_stddev_mistle_prune_list))
# print("stddev_mistle_prune_list = " + str(stddev_mistle_prune_list))
#
# # mean_mining4sat_list = [0.4070702040540324, 0.5024681934172461, 0.5510398349171806, 0.5813506739554395, 0.6054356949658861]
# # l_stddev_mining4sat_list = [0.35067417074627616, 0.4545828017203343, 0.5089644535055559, 0.537092571944983, 0.5648622433990004]
# # u_stddev_mining4sat_list = [0.4634662373617886, 0.550353585114158, 0.5931152163288053, 0.6256087759658959, 0.6460091465327719]
# # stddev_mining4sat_list = [0.056396033307756235, 0.04788539169691183, 0.04207538141162467, 0.04425810201045647, 0.04057345156688571]
# # mean_mistle_list = [0.47154270391996134, 0.5844683287934547, 0.6141024136903276, 0.6420873330763561, 0.6551141452371219]
# # l_stddev_mistle_list = [0.4059189729446973, 0.4357840689344571, 0.48150370969317646, 0.5119233433801365, 0.5288791887515998]
# # u_stddev_mistle_list = [0.5371664348952253, 0.7331525886524523, 0.7467011176874787, 0.7722513227725758, 0.7813491017226439]
# # stddev_mistle_list = [0.06562373097526401, 0.14868425985899758, 0.1325987039971511, 0.13016398969621965, 0.12623495648552213]
# # mean_mistle_prune_list = [0.5387116009523252, 0.6750507389576524, 0.7331817581125528, 0.7782333101150262, 0.8084391532208933]
# # l_stddev_mistle_prune_list = [0.4359190108009203, 0.5443145038349324, 0.623396392364384, 0.6824850133742515, 0.7200542639586766]
# # u_stddev_mistle_prune_list = [0.64150419110373, 0.8057869740803725, 0.8429671238607216, 0.8739816068558008, 0.8968240424831101]
# # stddev_mistle_prune_list = [0.10279259015140482, 0.1307362351227201, 0.10978536574816879, 0.09574829674077466, 0.08838488926221673]
#
# plt.figure(5)
# plt.ylabel("Compression\n(Literal Length)")
# plt.xlabel("Data Size")
#
# plt.plot(data_size_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
# plt.fill_between(
#     data_size_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
# )
# plt.plot(data_size_list, mean_mistle_list, marker="o", label="Mistle (without pruning)")
# plt.fill_between(data_size_list, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.3)
# plt.plot(
#     data_size_list, mean_mistle_prune_list, marker="o", label="Mistle (with pruning)"
# )
# plt.fill_between(
#     data_size_list, l_stddev_mistle_prune_list, u_stddev_mistle_prune_list, alpha=0.3
# )
#
# plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.savefig("Experiments/exp3_plot5_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

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


# data_size_list = [100, 200, 300, 400, 500]
# m4s_list = []
# m1_list = []
# m2_list = []
# m3_list = []
# for i in range(num_iterations):
#     m4s, m1, m2, m3 = plot7(data_size_list)
#     m4s_list.append(m4s)
#     m1_list.append(m1)
#     m2_list.append(m2)
#     m3_list.append(m3)
#
# m4s_list = np.array(m4s_list)
# m1_list = np.array(m1_list)
# m2_list = np.array(m2_list)
# m3_list = np.array(m3_list)
#
# mean_mining4sat_list = []
# l_stddev_mining4sat_list = []
# u_stddev_mining4sat_list = []
# stddev_mining4sat_list = []
#
# mean_mistle1_list = []
# l_stddev_mistle1_list = []
# u_stddev_mistle1_list = []
# stddev_mistle1_list = []
#
# mean_mistle2_list = []
# l_stddev_mistle2_list = []
# u_stddev_mistle2_list = []
# stddev_mistle2_list = []
#
# mean_mistle3_list = []
# l_stddev_mistle3_list = []
# u_stddev_mistle3_list = []
# stddev_mistle3_list = []
#
# for i in range(len(data_size_list)):
#     m = mean(m4s_list[:, i])
#     std_dev = stdev(m4s_list[:, i])
#     mean_mining4sat_list.append(m)
#     stddev_mining4sat_list.append(std_dev)
#     l_stddev_mining4sat_list.append(
#         max(m - std_dev, -1)
#     )  # Compression is a fraction between -1 to 1
#     u_stddev_mining4sat_list.append(min(m + std_dev, 1))
#
#     m = mean(m1_list[:, i])
#     std_dev = stdev(m1_list[:, i])
#     mean_mistle1_list.append(m)
#     stddev_mistle1_list.append(std_dev)
#     l_stddev_mistle1_list.append(max(m - std_dev, -1))
#     u_stddev_mistle1_list.append(min(m + std_dev, 1))
#
#     m = mean(m2_list[:, i])
#     std_dev = stdev(m2_list[:, i])
#     mean_mistle2_list.append(m)
#     stddev_mistle2_list.append(std_dev)
#     l_stddev_mistle2_list.append(max(m - std_dev, -1))
#     u_stddev_mistle2_list.append(min(m + std_dev, 1))
#
#     m = mean(m3_list[:, i])
#     std_dev = stdev(m3_list[:, i])
#     mean_mistle3_list.append(m)
#     stddev_mistle3_list.append(std_dev)
#     l_stddev_mistle3_list.append(max(m - std_dev, -1))
#     u_stddev_mistle3_list.append(min(m + std_dev, 1))
#
# print("mean_mining4sat_list = " + str(mean_mining4sat_list))
# print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
# print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
# print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))
#
# print("mean_mistle1_list = " + str(mean_mistle1_list))
# print("l_stddev_mistle1_list = " + str(l_stddev_mistle1_list))
# print("u_stddev_mistle1_list = " + str(u_stddev_mistle1_list))
# print("stddev_mistle1_list = " + str(stddev_mistle1_list))
#
# print("mean_mistle2_list = " + str(mean_mistle2_list))
# print("l_stddev_mistle2_list = " + str(l_stddev_mistle2_list))
# print("u_stddev_mistle2_list = " + str(u_stddev_mistle2_list))
# print("stddev_mistle2_list = " + str(stddev_mistle2_list))
#
# print("mean_mistle3_list = " + str(mean_mistle3_list))
# print("l_stddev_mistle3_list = " + str(l_stddev_mistle3_list))
# print("u_stddev_mistle3_list = " + str(u_stddev_mistle3_list))
# print("stddev_mistle3_list = " + str(stddev_mistle3_list))
#
#
# plt.figure(7)
# plt.ylabel("Compression\n(Literal Length)")
# plt.xlabel("Data Size")
#
# plt.plot(data_size_list, mean_mining4sat_list, marker="o", label="Mining4SAT")
# plt.fill_between(
#     data_size_list, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
# )
# plt.plot(data_size_list, mean_mistle1_list, marker="o", label="Mistle - W operator")
# plt.fill_between(
#     data_size_list, l_stddev_mistle1_list, u_stddev_mistle1_list, alpha=0.3
# )
# plt.plot(data_size_list, mean_mistle2_list, marker="o", label="Mistle - W, S operators")
# plt.fill_between(
#     data_size_list, l_stddev_mistle2_list, u_stddev_mistle2_list, alpha=0.3
# )
# plt.plot(
#     data_size_list,
#     mean_mistle3_list,
#     marker="o",
#     label="Mistle - all lossless operators",
# )
# plt.fill_between(
#     data_size_list, l_stddev_mistle3_list, u_stddev_mistle3_list, alpha=0.3
# )
#
# plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.savefig("Experiments/exp3_plot7_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()

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


# po_parameters = [0.5, 0.6, 0.7, 0.8, 0.9, 1]
# m4s_list = []
# m1_list = []
# m2_list = []
# m3_list = []
# for i in range(num_iterations):
#     m4s, m1, m2, m3 = plot8(po_parameters)
#     m4s_list.append(m4s)
#     m1_list.append(m1)
#     m2_list.append(m2)
#     m3_list.append(m3)
#
# m4s_list = np.array(m4s_list)
# m1_list = np.array(m1_list)
# m2_list = np.array(m2_list)
# m3_list = np.array(m3_list)
#
# mean_mining4sat_list = []
# l_stddev_mining4sat_list = []
# u_stddev_mining4sat_list = []
# stddev_mining4sat_list = []
#
# mean_mistle1_list = []
# l_stddev_mistle1_list = []
# u_stddev_mistle1_list = []
# stddev_mistle1_list = []
#
# mean_mistle2_list = []
# l_stddev_mistle2_list = []
# u_stddev_mistle2_list = []
# stddev_mistle2_list = []
#
# mean_mistle3_list = []
# l_stddev_mistle3_list = []
# u_stddev_mistle3_list = []
# stddev_mistle3_list = []
#
# for i in range(len(po_parameters)):
#     m = mean(m4s_list[:, i])
#     std_dev = stdev(m4s_list[:, i])
#     mean_mining4sat_list.append(m)
#     stddev_mining4sat_list.append(std_dev)
#     l_stddev_mining4sat_list.append(
#         max(m - std_dev, -1)
#     )  # Compression is a fraction between -1 to 1
#     u_stddev_mining4sat_list.append(min(m + std_dev, 1))
#
#     m = mean(m1_list[:, i])
#     std_dev = stdev(m1_list[:, i])
#     mean_mistle1_list.append(m)
#     stddev_mistle1_list.append(std_dev)
#     l_stddev_mistle1_list.append(max(m - std_dev, -1))
#     u_stddev_mistle1_list.append(min(m + std_dev, 1))
#
#     m = mean(m2_list[:, i])
#     std_dev = stdev(m2_list[:, i])
#     mean_mistle2_list.append(m)
#     stddev_mistle2_list.append(std_dev)
#     l_stddev_mistle2_list.append(max(m - std_dev, -1))
#     u_stddev_mistle2_list.append(min(m + std_dev, 1))
#
#     m = mean(m3_list[:, i])
#     std_dev = stdev(m3_list[:, i])
#     mean_mistle3_list.append(m)
#     stddev_mistle3_list.append(std_dev)
#     l_stddev_mistle3_list.append(max(m - std_dev, -1))
#     u_stddev_mistle3_list.append(min(m + std_dev, 1))
#
# print("mean_mining4sat_list = " + str(mean_mining4sat_list))
# print("l_stddev_mining4sat_list = " + str(l_stddev_mining4sat_list))
# print("u_stddev_mining4sat_list = " + str(u_stddev_mining4sat_list))
# print("stddev_mining4sat_list = " + str(stddev_mining4sat_list))
#
# print("mean_mistle1_list = " + str(mean_mistle1_list))
# print("l_stddev_mistle1_list = " + str(l_stddev_mistle1_list))
# print("u_stddev_mistle1_list = " + str(u_stddev_mistle1_list))
# print("stddev_mistle1_list = " + str(stddev_mistle1_list))
#
# print("mean_mistle2_list = " + str(mean_mistle2_list))
# print("l_stddev_mistle2_list = " + str(l_stddev_mistle2_list))
# print("u_stddev_mistle2_list = " + str(u_stddev_mistle2_list))
# print("stddev_mistle2_list = " + str(stddev_mistle2_list))
#
# print("mean_mistle3_list = " + str(mean_mistle3_list))
# print("l_stddev_mistle3_list = " + str(l_stddev_mistle3_list))
# print("u_stddev_mistle3_list = " + str(u_stddev_mistle3_list))
# print("stddev_mistle3_list = " + str(stddev_mistle3_list))
#
# # version 4; 10 iterations
# # mean_mining4sat_list = [0.36167005006254194, 0.40379115233541973, 0.4518806845933667, 0.49461657143786464, 0.5634443854200426, 0.6457999999999999]
# # l_stddev_mining4sat_list = [0.3433369129300138, 0.385371911728452, 0.4360789210588824, 0.48632385046945686, 0.5470194282632608, 0.6101779874546962]
# # u_stddev_mining4sat_list = [0.3800031871950701, 0.4222103929423875, 0.46768244812785104, 0.5029092924062724, 0.5798693425768244, 0.6814220125453037]
# # stddev_mining4sat_list = [0.018333137132528182, 0.01841924060696776, 0.015801763534484328, 0.008292720968407774, 0.016424957156781862, 0.03562201254530376]
# # mean_mistle1_list = [0.32956795345154516, 0.35044686955183313, 0.37414046538068735, 0.4083294915742425, 0.4800988057286257, 0.639425]
# # l_stddev_mistle1_list = [0.32200944893253575, 0.3407565174517392, 0.364216829414351, 0.39715214502954443, 0.4595912733818397, 0.624029270974643]
# # u_stddev_mistle1_list = [0.33712645797055457, 0.3601372216519271, 0.3840641013470237, 0.4195068381189405, 0.5006063380754117, 0.654820729025357]
# # stddev_mistle1_list = [0.0075585045190094055, 0.009690352100093952, 0.009923635966336341, 0.011177346544698053, 0.02050753234678601, 0.015395729025357067]
# # mean_mistle2_list = [0.5089518037468755, 0.3833015453007639, 0.3796115388821686, 0.4083294915742425, 0.4800988057286257, 0.639425]
# # l_stddev_mistle2_list = [0.3677263366514345, 0.3399859486958648, 0.36359324869099696, 0.39715214502954443, 0.4595912733818397, 0.624029270974643]
# # u_stddev_mistle2_list = [0.6501772708423166, 0.42661714190566297, 0.3956298290733402, 0.4195068381189405, 0.5006063380754117, 0.654820729025357]
# # stddev_mistle2_list = [0.141225467095441, 0.04331559660489907, 0.016018290191171618, 0.011177346544698053, 0.02050753234678601, 0.015395729025357067]
# # mean_mistle3_list = [0.7917470198305913, 0.5273343129727289, 0.4185614675683649, 0.4083294915742425, 0.4800988057286257, 0.639425]
# # l_stddev_mistle3_list = [0.6819892002850368, 0.41887715063540787, 0.33444936345865456, 0.39715214502954443, 0.4595912733818397, 0.624029270974643]
# # u_stddev_mistle3_list = [0.9015048393761457, 0.63579147531005, 0.5026735716780752, 0.4195068381189405, 0.5006063380754117, 0.654820729025357]
# # stddev_mistle3_list = [0.10975781954555439, 0.10845716233732104, 0.08411210410971032, 0.011177346544698053, 0.02050753234678601, 0.015395729025357067]
#
# plt.figure(8)
# plt.ylabel("Compression\n(Literal Length)")
# plt.xlabel("Data Size")
#
# plt.plot(po_parameters, mean_mining4sat_list, marker="o", label="Mining4SAT")
# plt.fill_between(
#     po_parameters, l_stddev_mining4sat_list, u_stddev_mining4sat_list, alpha=0.3
# )
# plt.plot(po_parameters, mean_mistle1_list, marker="o", label="Mistle - W operator")
# plt.fill_between(po_parameters, l_stddev_mistle1_list, u_stddev_mistle1_list, alpha=0.3)
# plt.plot(po_parameters, mean_mistle2_list, marker="o", label="Mistle - W, S operators")
# plt.fill_between(po_parameters, l_stddev_mistle2_list, u_stddev_mistle2_list, alpha=0.3)
# plt.plot(
#     po_parameters,
#     mean_mistle3_list,
#     marker="o",
#     label="Mistle - all lossless operators",
# )
# plt.fill_between(po_parameters, l_stddev_mistle3_list, u_stddev_mistle3_list, alpha=0.3)
#
# plt.legend(bbox_to_anchor=(0, -0.2), loc="upper left")
# plt.savefig("Experiments/exp3_plot8_v" + str(version) + ".pdf", bbox_inches="tight")
# plt.show()
# plt.close()
