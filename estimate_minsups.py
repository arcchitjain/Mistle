from mistle_v2 import load_data, get_topk_minsup
from pattern_mining import compute_itemsets

dataset_class_vars = {
    "iris_17.dat": (17, 18),
    "iris_18.dat": (17, 18),
    "iris_19.dat": (17, 18),
    "glass.dat": (42, 43),
    "wine.dat": (66, 67),
    "ecoli.dat": (27, 28),
    "hepatitis.dat": (55, 56),
    "heart.dat": (48, 49),
    "dermatology.dat": (44, 45),
    "auto.dat": (132, 133),
    "horseColic.dat": (84, 85),
    "pima.dat": (37, 38),
    "ticTacToe.dat": (28, 29),
    "ionosphere.dat": (156, 157),
    "flare.dat": (31, 32),
    "cylBands.dat": (123, 124),
    "led.dat": (15, 16),
}

minsup_dict = {}
int_minsup_dict = {}
nb_freq_items = {}
for data, class_vars in dataset_class_vars.items():
    name = data.split(".")[0]
    print(
        "\n\n####################################### "
        + name
        + " #######################################"
    )
    pos, neg = load_data(data, class_vars)
    r = get_topk_minsup(pos + neg, topk=20000, suppress_output=True)
    freq_items_lcm = compute_itemsets(pos + neg, r, "LCM", suppress_output=True)
    int_minsup = round(r * (len(pos) + len(neg)))
    print("Optimal Minsup for " + str(name) + ": " + str(r))
    nb_freq_items[data] = len(freq_items_lcm)
    minsup_dict[data] = r
    int_minsup_dict[data] = int_minsup

print("nb_freq_items = " + str(nb_freq_items))
print("minsup_dict = " + str(minsup_dict))
print("int_minsup_dict = " + str(int_minsup_dict))
