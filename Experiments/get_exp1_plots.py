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
lylim = 0.8
uylim = 1.0
version = 4

fig = plt.figure(figsize=(10, 1.5))

# fig, (ax1, ax2, ax3) = plt.subplots(1, 3, sharey=True, figsize=(9, 3))

ax1 = fig.add_axes([0.075, 0.2, 0.275, 0.75])
ax1.set_ylabel("Accuracy")
ax1.set_ylim(bottom=lylim, top=uylim)
ax1.set_yticks([0.8, 0.9, 1])
ax1.set_yticklabels(["0.8", "0.9", "1.0"])
ax1.set_xticks([100, 200, 300, 400, 500])
ax1.set_xticklabels(["100", "200", "300", "400", "500"])
ax1.set_xlabel("Number of examples")
data_sizes = [100, 150, 200, 250, 300, 350, 400, 450, 500]
mean_similarity_list1 = [
    0.970703125,
    0.9612630208333334,
    0.9879557291666666,
    0.9887152777777778,
    0.98828125,
    0.9928385416666666,
    0.9780815972222222,
    0.9621310763888888,
    0.9930555555555556,
]
lstd_dev_similarity_list1 = [
    0.9413006460599129,
    0.9168347931353727,
    0.9687879689846446,
    0.9733665303661622,
    0.9724666652922835,
    0.9815445636332483,
    0.9570168525842913,
    0.9191490733230527,
    0.9856271872565686,
]
ustd_dev_similarity_list1 = [1, 1, 1, 1, 1, 1, 0.9991463418601532, 1, 1]
ax1.plot(data_sizes, mean_similarity_list1, marker="o")
ax1.fill_between(
    data_sizes, lstd_dev_similarity_list1, ustd_dev_similarity_list1, alpha=0.3
)

ax2 = fig.add_axes([0.4, 0.2, 0.275, 0.75])
ax2.set_xlabel("Missingness Parameter")
ax2.set_xticks([0, 0.1, 0.2, 0.3, 0.4, 0.5])
ax2.set_xticklabels(["0", "0.1", "0.2", "0.3", "0.4", "0.5"])
ax2.set_ylim(bottom=lylim, top=uylim)
ax2.set_yticks([0.8, 0.9, 1])
ax2.set_yticklabels([])
missingness_paramaters = [0, 0.1, 0.2, 0.3, 0.4, 0.5]
mean_similarity_list2 = [
    0.9915364583333334,
    0.9873046875,
    0.9588216145833334,
    0.9892578125,
    0.9934895833333334,
    0.9938151041666666,
]
lstd_dev_similarity_list2 = [
    0.9794702128952796,
    0.9719505906195051,
    0.8887035656825196,
    0.976184999585405,
    0.9862428140055498,
    0.9876800942763595,
]
ustd_dev_similarity_list2 = [1, 1, 1, 1, 1, 0.9999501140569738]
ax2.plot(missingness_paramaters, mean_similarity_list2, marker="o")
ax2.fill_between(
    missingness_paramaters,
    lstd_dev_similarity_list2,
    ustd_dev_similarity_list2,
    alpha=0.3,
)

ax3 = fig.add_axes([0.725, 0.2, 0.3, 0.75])
ax3.set_xlabel("Number of variables")
ax3.set_xticks([4, 6, 8, 10, 12])
ax3.set_xticklabels(["4", "6", "8", "10", "12"])
ax3.set_yticks([0.8, 0.9, 1])
ax3.set_yticklabels([])
ax3.set_ylim(bottom=lylim, top=uylim)
alphabet_sizes = [4, 6, 8, 10, 12]
mean_similarity_list3 = [
    0.995849609375,
    0.98212890625,
    0.98037109375,
    0.9890625,
    0.99892578125,
]
lstd_dev_similarity_list3 = [
    0.9897071554563432,
    0.959844871234933,
    0.9524569467951004,
    0.971236555923274,
    0.9965237551022951,
]
ustd_dev_similarity_list3 = [1, 1, 1, 1, 1]
ax3.plot(alphabet_sizes, mean_similarity_list3, marker="o")
ax3.fill_between(
    alphabet_sizes, lstd_dev_similarity_list3, ustd_dev_similarity_list3, alpha=0.3
)

plt.savefig("exp1_plots_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()
