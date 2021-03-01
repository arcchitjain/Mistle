from mistle_v2 import load_data

dataset_class_vars = {
    "iris_17.dat": (17, 18),
    "iris_18.dat": (17, 18),
    "iris_19.dat": (17, 18),
    "zoo.dat": (36, 37),
    "glass.dat": (42, 43),
    "wine.dat": (66, 67),
    "ecoli.dat": (27, 28),
    "hepatitis.dat": (55, 56),
    "heart.dat": (48, 49),
    "dermatology.dat": (44, 45),
    "auto.dat": (132, 133),
    "breast.dat": (19, 20),
    "horseColic.dat": (84, 85),
    "pima.dat": (37, 38),
    "congres.dat": (33, 34),
    "ticTacToe.dat": (28, 29),
    "ionosphere.dat": (156, 157),
    "flare.dat": (31, 32),
    "cylBands.dat": (123, 124),
    "led.dat": (15, 16),
    "soyabean.dat": (100, 101),
}

for data, class_vars in dataset_class_vars.items():
    name = data.split(".")[0]
    print(
        "\n\n####################################### "
        + name
        + " #######################################"
    )
    pos, neg = load_data(data, class_vars)
    pos = set(pos)
    neg = set(neg)
    inconsistent_pas = pos & neg
    print(str(len(inconsistent_pas)) + " inconsistencies found.")
