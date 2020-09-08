import matplotlib
import matplotlib.pyplot as plt

plt.style.use("seaborn")

matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=20)
matplotlib.rc("axes", titlesize=14)
matplotlib.rc("axes", labelsize=18)
matplotlib.rc("xtick", labelsize=18)
matplotlib.rc("ytick", labelsize=18)
matplotlib.rc("legend", fontsize=14)
matplotlib.rc("figure", titlesize=14)
lylim = 0.5
uylim = 1.0
version = 4

fig = plt.figure(figsize=(10, 3))

# fig, (ax1, ax2, ax3) = plt.subplots(1, 3, sharey=True, figsize=(9, 3))

ax1 = fig.add_axes([0.075, 0.2, 0.275, 0.75])
ax1.set_ylabel("Accuracy")
ax1.set_ylim(bottom=lylim, top=uylim)
ax1.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax1.set_yticklabels(["0", "0.2", "0.4", "0.6", "0.8", "1.0"])
ax1.set_xticks([100, 200, 300, 400, 500])
ax1.set_xticklabels(["100", "200", "300", "400", "500"])
ax1.set_xlabel("Number of examples")
# data_sizes = [100, 150, 200, 250, 300, 350, 400, 450, 500]
# mean_similarity_list1 = [
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
# lstd_dev_similarity_list1 = [
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
# ustd_dev_similarity_list1 = [1, 1, 1, 1, 1, 1, 0.9991463418601532, 1, 1]

data_sizes = [100, 150, 200, 250, 300, 350, 400, 450, 500]
mean_similarity_list1 = [
    0.8685711669921875,
    0.8943780517578125,
    0.9211541748046875,
    0.89572509765625,
    0.8922027587890625,
    0.913797607421875,
    0.9280670166015625,
    0.9300970458984374,
    0.9351483154296875,
]
std_dev_similarity_list1 = [
    0.21013777824796837,
    0.1697991198018499,
    0.16814221542075158,
    0.23031493739608433,
    0.24800759418762414,
    0.21276205626494404,
    0.1925880368252782,
    0.19255174592500943,
    0.19311111716360568,
]
lstd_dev_similarity_list1 = [
    0.6584333887442191,
    0.7245789319559626,
    0.753011959383936,
    0.6654101602601656,
    0.6441951646014383,
    0.701035551156931,
    0.7354789797762844,
    0.737545299973428,
    0.7420371982660818,
]
ustd_dev_similarity_list1 = [1, 1, 1, 1, 1, 1, 1, 1, 1]


ax1.plot(data_sizes, mean_similarity_list1, marker="o")
ax1.fill_between(
    data_sizes, lstd_dev_similarity_list1, ustd_dev_similarity_list1, alpha=0.3
)

ax2 = fig.add_axes([0.4, 0.2, 0.275, 0.75])
ax2.set_xlabel("Missingness Parameter")
ax2.set_xticks([0, 0.1, 0.2, 0.3, 0.4, 0.5])
ax2.set_xticklabels(["0", "0.1", "0.2", "0.3", "0.4", "0.5"])
ax2.set_ylim(bottom=lylim, top=uylim)
ax2.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax2.set_yticklabels([])
# missingness_parameters = [0, 0.1, 0.2, 0.3, 0.4, 0.5]
# mean_similarity_list2 = [
#     0.9915364583333334,
#     0.9873046875,
#     0.9588216145833334,
#     0.9892578125,
#     0.9934895833333334,
#     0.9938151041666666,
# ]
# lstd_dev_similarity_list2 = [
#     0.9794702128952796,
#     0.9719505906195051,
#     0.8887035656825196,
#     0.976184999585405,
#     0.9862428140055498,
#     0.9876800942763595,
# ]
# ustd_dev_similarity_list2 = [1, 1, 1, 1, 1, 0.9999501140569738]

missingness_parameters = [0, 0.1, 0.2, 0.3, 0.4, 0.5]
mean_similarity_list2 = [
    0.9105499267578125,
    0.8725616455078125,
    0.8701470947265625,
    0.9439141845703125,
    0.932012939453125,
    0.9054693603515624,
]
std_dev_similarity_list2 = [
    0.2343597626107986,
    0.2933108497776877,
    0.29589587458192945,
    0.19426318422022765,
    0.21560589764435656,
    0.2688686071510578,
]
lstd_dev_similarity_list2 = [
    0.6761901641470139,
    0.5792507957301247,
    0.574251220144633,
    0.7496510003500849,
    0.7164070418087685,
    0.6366007532005047,
]
ustd_dev_similarity_list2 = [1, 1, 1, 1, 1, 1]

ax2.plot(missingness_parameters, mean_similarity_list2, marker="o")
ax2.fill_between(
    missingness_parameters,
    lstd_dev_similarity_list2,
    ustd_dev_similarity_list2,
    alpha=0.3,
)

ax3 = fig.add_axes([0.725, 0.2, 0.35, 0.75])
ax3.set_xlabel("Number of variables")
ax3.set_xticks([4, 6, 8, 10, 12, 14, 16])
ax3.set_xticklabels(["4", "6", "8", "10", "12", "14", "16"])
ax3.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax3.set_yticklabels([])
ax3.set_ylim(bottom=lylim, top=uylim)
# alphabet_sizes = [4, 6, 8, 10, 12]
# mean_similarity_list3 = [
#     0.995849609375,
#     0.98212890625,
#     0.98037109375,
#     0.9890625,
#     0.99892578125,
# ]
# lstd_dev_similarity_list3 = [
#     0.9897071554563432,
#     0.959844871234933,
#     0.9524569467951004,
#     0.971236555923274,
#     0.9965237551022951,
# ]
# ustd_dev_similarity_list3 = [1, 1, 1, 1, 1]
alphabet_sizes = [4, 6, 8, 10, 12, 14, 16]
mean_similarity_list3 = [
    0.98,
    0.979375,
    0.986484375,
    0.90728515625,
    0.94330322265625,
    0.92934814453125,
    0.9324873352050781,
]
std_dev_similarity_list3 = [
    0.14070529413628968,
    0.14066673062076673,
    0.09993694128236881,
    0.26940701808792844,
    0.19449554624213153,
    0.19285295956405774,
    0.167992814740012,
]
lstd_dev_similarity_list3 = [
    0.8392947058637104,
    0.8387082693792333,
    0.8865474337176311,
    0.6378781381620715,
    0.7488076764141185,
    0.7364951849671924,
    0.7644945204650662,
]
ustd_dev_similarity_list3 = [1, 1, 1, 1, 1, 1, 1]

ax3.plot(alphabet_sizes, mean_similarity_list3, marker="o")
ax3.fill_between(
    alphabet_sizes, lstd_dev_similarity_list3, ustd_dev_similarity_list3, alpha=0.3
)

plt.savefig("exp1_plots_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()
