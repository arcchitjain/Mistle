import matplotlib
import matplotlib.pyplot as plt

plt.style.use("seaborn")
matplotlib.rcParams["mathtext.fontset"] = "stix"
matplotlib.rcParams["font.family"] = "STIXGeneral"
matplotlib.rc("font", size=22)
matplotlib.rc("axes", titlesize=16)
matplotlib.rc("axes", labelsize=28)
matplotlib.rc("xtick", labelsize=22)
matplotlib.rc("ytick", labelsize=22)
matplotlib.rc("legend", fontsize=20)
matplotlib.rc("figure", titlesize=16)
lylim = 0.5
uylim = 1.0
version = 5

fig = plt.figure(figsize=(10, 4))

# fig, (ax1, ax2, ax3) = plt.subplots(1, 3, sharey=True, figsize=(9, 3))

ax1 = fig.add_axes([0.11, 0.2, 0.4, 0.75])
ax1.set_ylabel("Compression")
ax1.set_ylim(bottom=lylim, top=uylim)
ax1.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax1.set_yticklabels(["0.0", "0.2", "0.4", "0.6", "0.8", "1.0"])
ax1.set_xticks([0.05, 0.1, 0.15, 0.2, 0.25, 0.3])
ax1.set_xticklabels(["0.05", "0.10", "0.15", "0.20", "0.25", "0.30"])
ax1.set_xlabel("Minimum Support Threshold")
minsups = [0.05, 0.1, 0.15, 0.2, 0.25, 0.3]

mean_mining4sat_list = [
    0.5308517044630887,
    0.4759440853010015,
    0.4217773666562703,
    0.3584949553384384,
    0.19162045415675605,
    0.10470237599836019,
]
l_stddev_mining4sat_list = [
    0.47220980076381663,
    0.4144441885517034,
    0.34788779407969517,
    0.3001622560098504,
    0.09602451364613619,
    0.007897513610471196,
]
u_stddev_mining4sat_list = [
    0.5894936081623607,
    0.5374439820502996,
    0.49566693923284544,
    0.41682765466702637,
    0.2872163946673759,
    0.20150723838624918,
]
mean_mistle_list = [
    0.5613033254066407,
    0.5322241170171031,
    0.5046815629320083,
    0.40898118169886544,
    0.27307284390379,
    0.21860527351103137,
]
l_stddev_mistle_list = [
    0.4115611946278379,
    0.3722644256263424,
    0.3386085024558574,
    0.20971637049993458,
    0.018476732791587402,
    -0.055448281029083335,
]
u_stddev_mistle_list = [
    0.7110454561854436,
    0.692183808407864,
    0.6707546234081592,
    0.6082459928977964,
    0.5276689550159925,
    0.4926588280511461,
]
mean_mistle_prune_list = [
    0.746942989841752,
    0.7332038669186653,
    0.7234773452602723,
    0.683848018055645,
    0.5832993529506035,
    0.4599106878781747,
]
l_stddev_mistle_prune_list = [
    0.6431614722294856,
    0.625952585841419,
    0.6120191341041247,
    0.5587273091413096,
    0.37513904955734706,
    0.13357037486110696,
]
u_stddev_mistle_prune_list = [
    0.8507245074540184,
    0.8404551479959117,
    0.8349355564164199,
    0.8089687269699803,
    0.7914596563438601,
    0.7862510008952424,
]

ax1.plot(
    minsups,
    l_stddev_mining4sat_list,
    linestyle="--",
    label="_nolegend_",
    color="b",
    alpha=0.3,
)
ax1.plot(
    minsups,
    u_stddev_mining4sat_list,
    linestyle="--",
    label="_nolegend_",
    color="b",
    alpha=0.3,
)
ax1.plot(
    minsups,
    l_stddev_mistle_list,
    linestyle=":",
    label="_nolegend_",
    color="g",
    alpha=0.3,
)
ax1.plot(
    minsups,
    u_stddev_mistle_list,
    linestyle=":",
    label="_nolegend_",
    color="g",
    alpha=0.3,
)
ax1.plot(
    minsups,
    l_stddev_mistle_prune_list,
    linestyle="-.",
    label="_nolegend_",
    color="r",
    alpha=0.3,
)
ax1.plot(
    minsups,
    u_stddev_mistle_prune_list,
    linestyle="-.",
    label="_nolegend_",
    color="r",
    alpha=0.3,
)
ax1.plot(
    minsups,
    mean_mining4sat_list,
    marker="o",
    markersize=15,
    label="Mining4SAT",
    color="b",
    alpha=1,
)
ax1.fill_between(
    minsups, l_stddev_mining4sat_list, u_stddev_mining4sat_list, color="b", alpha=0.2
)

ax1.plot(
    minsups,
    mean_mistle_list,
    marker="^",
    markersize=15,
    label="Mistle (without pruning)",
    color="g",
    alpha=1,
)
ax1.fill_between(
    minsups, l_stddev_mistle_list, u_stddev_mistle_list, color="g", alpha=0.2
)

ax1.plot(
    minsups,
    mean_mistle_prune_list,
    marker="s",
    markersize=15,
    label="Mistle",
    color="r",
    alpha=1,
)
ax1.fill_between(
    minsups,
    l_stddev_mistle_prune_list,
    u_stddev_mistle_prune_list,
    color="r",
    alpha=0.2,
)


ax1.legend(bbox_to_anchor=(-0.05, 1.02), loc="lower left", ncol=3)

ax2 = fig.add_axes([0.55, 0.2, 0.4, 0.75])
ax2.set_xlabel("Missingness Parameter")
ax2.set_xticks([0, 0.1, 0.2, 0.3, 0.4, 0.5])
ax2.set_xticklabels(["0", "0.1", "0.2", "0.3", "0.4", "0.5"])
ax2.set_ylim(bottom=lylim, top=uylim)
ax2.set_yticks([0, 0.2, 0.4, 0.6, 0.8, 1])
ax2.set_yticklabels([])

po_paramaters = [0.5, 0.6, 0.7, 0.8, 0.9, 1]
missingness_paramaters = [0.5, 0.4, 0.3, 0.2, 0.1, 0]
mean_mining4sat_list = [
    0.36167005006254194,
    0.40379115233541973,
    0.4518806845933667,
    0.49461657143786464,
    0.5634443854200426,
    0.6457999999999999,
]
l_stddev_mining4sat_list = [
    0.3433369129300138,
    0.385371911728452,
    0.4360789210588824,
    0.48632385046945686,
    0.5470194282632608,
    0.6101779874546962,
]
u_stddev_mining4sat_list = [
    0.3800031871950701,
    0.4222103929423875,
    0.46768244812785104,
    0.5029092924062724,
    0.5798693425768244,
    0.6814220125453037,
]
stddev_mining4sat_list = [
    0.018333137132528182,
    0.01841924060696776,
    0.015801763534484328,
    0.008292720968407774,
    0.016424957156781862,
    0.03562201254530376,
]
mean_mistle_list = [
    0.8346818236198736,
    0.597390025366827,
    0.5029121262975371,
    0.5054118187431345,
    0.5759738878551149,
    0.719575,
]
l_stddev_mistle_list = [
    0.7378247908483149,
    0.49626203347980746,
    0.4326470884245695,
    0.4896849051892896,
    0.5554313900456025,
    0.7063310024077991,
]
u_stddev_mistle_list = [
    0.9315388563914324,
    0.6985180172538464,
    0.5731771641705048,
    0.5211387322969793,
    0.5965163856646274,
    0.7328189975922008,
]
stddev_mistle_list = [
    0.09685703277155877,
    0.10112799188701949,
    0.07026503787296762,
    0.015726913553844857,
    0.020542497809512422,
    0.013243997592200876,
]
mean_mistle_prune_list = [
    0.9760345463836486,
    0.9269325576325638,
    0.8509223550207345,
    0.7611211588426247,
    0.7125971711207684,
    0.719575,
]
l_stddev_mistle_prune_list = [
    0.9584875936947781,
    0.8967162269003766,
    0.8016757667279598,
    0.7253983294945825,
    0.683946466649498,
    0.7063310024077991,
]
u_stddev_mistle_prune_list = [
    0.9935814990725191,
    0.957148888364751,
    0.9001689433135093,
    0.7968439881906668,
    0.7412478755920388,
    0.7328189975922008,
]
stddev_mistle_prune_list = [
    0.01754695268887054,
    0.03021633073218717,
    0.049246588292774736,
    0.03572282934804222,
    0.028650704471270425,
    0.013243997592200876,
]

ax2.plot(
    missingness_paramaters,
    l_stddev_mining4sat_list,
    linestyle="--",
    color="b",
    alpha=0.3,
)
ax2.plot(
    missingness_paramaters,
    u_stddev_mining4sat_list,
    linestyle="--",
    color="b",
    alpha=0.3,
)
ax2.plot(
    missingness_paramaters, l_stddev_mistle_list, linestyle=":", color="g", alpha=0.3
)
ax2.plot(
    missingness_paramaters, u_stddev_mistle_list, linestyle=":", color="g", alpha=0.3
)
ax2.plot(
    missingness_paramaters,
    l_stddev_mistle_prune_list,
    linestyle="-.",
    color="r",
    alpha=0.3,
)
ax2.plot(
    missingness_paramaters,
    u_stddev_mistle_prune_list,
    linestyle="-.",
    color="r",
    alpha=0.3,
)

ax2.plot(
    missingness_paramaters,
    mean_mining4sat_list,
    marker="o",
    markersize=15,
    color="b",
    alpha=1,
)
ax2.fill_between(
    missingness_paramaters,
    l_stddev_mining4sat_list,
    u_stddev_mining4sat_list,
    alpha=0.2,
)

ax2.plot(
    missingness_paramaters,
    mean_mistle_list,
    marker="^",
    markersize=15,
    color="g",
    alpha=1,
)
ax2.fill_between(
    missingness_paramaters, l_stddev_mistle_list, u_stddev_mistle_list, alpha=0.2
)

ax2.plot(
    missingness_paramaters,
    mean_mistle_prune_list,
    marker="s",
    markersize=15,
    color="r",
    alpha=1,
)
ax2.fill_between(
    missingness_paramaters,
    l_stddev_mistle_prune_list,
    u_stddev_mistle_prune_list,
    alpha=0.2,
)


plt.savefig("exp3_plots_v" + str(version) + ".pdf", bbox_inches="tight")
plt.show()
plt.close()
