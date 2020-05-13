import uuid
import os


def run_mining4sat(
    transactions,
    support,
    code_path=os.path.abspath("Resources/Mining4SAT"),
    n=False,
    b=False,
):
    """
    Computes itemsets from a set of clauses.
    :param support: Minimum support threshold of the itemset, greater than 1.
    An itemset is consider to be frequent/interesting if its support is greater than or equal to this minimum support threshold.
    :param transactions: A list of clauses. Clauses are represented by a list of literals. This is like the DIMACS format.
    :param code_path: absolute path to the code of mining4SAT
    :param n: If True, then launch only non-binary reductions
    :param b: If True, then launch only binary reductions
    :return: compressed theory/cnfs as a list of clauses
    """
    working_dir = os.getcwd()
    rand_id = uuid.uuid4()
    dataset_name = os.path.join(working_dir, "mining4sat_" + str(rand_id) + "_.cnf")
    print("Input Data\t: " + str(dataset_name))

    with open(dataset_name, "w+") as db_file:
        db_file.write(
            "p cnf " + str(len(transactions)) + " " + str(len(transactions)) + "\n"
        )
        db_file.write(
            "\n".join(
                [
                    " ".join([str(item) for item in transaction]) + " 0"
                    for transaction in transactions
                ]
            )
        )

    if n is True and b is True:
        # It is absurd to run Mining4SAT with both the options True
        # Reverting back to the default setting of both being False
        n = False
        b = False

    if n is True:
        result = os.system(
            "cd "
            + code_path
            + " && java -Xms8G -Xmx25G -cp bin/ execution.FrequentPatterns "
            # + dataset_base_name
            # + ".cnf "
            + dataset_name
            + " "
            + str(support)
            + " -n"
            # + " >/dev/null 2>&1"      # Suppress both stdout and stderr
            + " 2>&1"  # Suppress only stderr
        )
        output_prefix = "NonBinary_Compressed_"
    elif b is True:
        result = os.system(
            "cd "
            + code_path
            + " && java -Xms8G -Xmx25G -cp bin/ execution.FrequentPatterns "
            # + dataset_base_name
            # + ".cnf "
            + dataset_name
            + " "
            + str(support)
            + " -b"
            # + " >/dev/null 2>&1"      # Suppress both stdout and stderr
            + " 2>&1"  # Suppress only stderr
        )
        output_prefix = "Binary_Compressed_"
    else:

        print(
            "Command \t: "
            + " ".join(
                [
                    "cd",
                    code_path,
                    "&&",
                    "java",
                    "-Xms8G",
                    "-Xmx25G",
                    "-cp",
                    "bin/",
                    "execution.FrequentPatterns",
                    # dataset_base_name + ".cnf",
                    dataset_name,
                    str(support),
                ]
            )
        )
        result = os.system(
            "cd "
            + code_path
            + " && java -Xms8G -Xmx25G -cp bin/ execution.FrequentPatterns "
            # + dataset_base_name
            # + ".cnf "
            + dataset_name
            + " "
            + str(support)
            # + " >/dev/null 2>&1"      # Suppress both stdout and stderr
            + " 2>&1"  # Suppress only stderr
        )
        output_prefix = "Binary_NonBinary_Compressed_"

    print("Return Code\t: " + str(result))

    output_base_name = (
        output_prefix + "mining4sat_" + str(rand_id) + "_L" + str(support) + ".cnf"
    )
    output_name = os.path.join(code_path, output_base_name)

    compressed_theory = []
    try:
        if (
            not os.path.exists(output_name)
            and "Binary_NonBinary" == output_base_name[:16]
        ):
            # This case occurs when there is no binary clause in the input file
            output_base_name = output_base_name[7:]
            output_name = os.path.join(code_path, output_base_name)

        if not os.path.exists(output_name) and "NonBinary" == output_base_name[:9]:
            # This case occurs when there is no non binary clause in the input file
            output_base_name = output_base_name[3:]
            output_name = os.path.join(code_path, output_base_name)

        if not os.path.exists(output_name):
            output_names = output_name.split("_")
            output_name = "_".join(output_names[:-1]) + "_.cnf"

        print("Output Data\t: " + str(output_name))

        with open(output_name) as output:
            output_lines = output.readlines()
            for l in output_lines[1:]:
                items = l.split()
                compressed_theory.append(items[:-1])

        os.remove(output_name)
    except FileNotFoundError as e:
        print("Output File not created\t: " + str(e))

    os.remove(dataset_name)
    return compressed_theory


if __name__ == "__main__":
    # Testing the code
    transactions = []
    transactions.append("1 2 3".split(" "))
    transactions.append("1 2 3 4".split(" "))
    transactions.append("1 2 3 4 5".split(" "))
    transactions.append("1 2".split(" "))
    transactions.append("1 3".split(" "))
    transactions.append("1 4".split(" "))
    transactions.append("1 5".split(" "))
    transactions.append("2 3".split(" "))
    transactions.append("2 4".split(" "))
    transactions.append("2 5".split(" "))
    transactions.append("3 4".split(" "))
    transactions.append("3 5".split(" "))
    transactions.append("4 5".split(" "))

    support = 3

    compressed_theory = run_mining4sat(transactions, support)

    print("Compressed Theory \t\t:" + str(compressed_theory))
