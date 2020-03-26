import uuid
import os
import subprocess


def compute_itemsets(transactions, support, algo="LCM", spmf_path="Resources/spmf.jar"):
    """
    Computes itemsets from a set of clauses. Depending on the algorithm, itemsets can be closed, maximal or only frequent
    :param support: Support of the itemset, between à and 1. It is relative support.
    :param transactions: A list of clauses. Clauses are represented by a list of literals. This is like the DIMACS format
    :param algo: Name of the algorithm (see spmf doc for more details). LCM is for closed itemsets, FPMax for maximal, Eclat for frequent.
    :param spmf_path: path to the spmf.jar file
    :return: a list of patterns as a list of tuples. First element of the tuple is the items of the patterns, second element is the support.
    """
    working_dir = os.getcwd()
    rand_id = uuid.uuid4()
    dataset_base_name = "temp_spmf_dataset" + str(rand_id)
    dataset_name = os.path.join(working_dir, dataset_base_name)
    output_base_name = "temp_spmf_dataset_res" + str(rand_id)
    output_name = os.path.join(working_dir, output_base_name)

    result_patterns = []

    encoding_dict = {}

    encoded_transactions = []
    for transaction in transactions:
        encoded_transaction = []
        for item in transaction:
            if item not in encoding_dict:
                encoding_dict[item] = len(encoding_dict)+1
            encoded_transaction.append(encoding_dict[item])
        encoded_transactions.append(sorted(encoded_transaction))

    inverse_token_dict = {v: k for k, v in encoding_dict.items()}

    with open(dataset_name, "w+") as db_file:
        db_file.write("\n".join([" ".join([str(item) for item in transaction]) for transaction in encoded_transactions]))

    subprocess.call(["java", "-jar", spmf_path, "run", algo, dataset_name, output_name, str(support)])

    with open(output_name) as output:
        for l in output:
            pattern, support = l.split("#SUP:")
            support = int(support.strip())
            items = pattern.split()
            decoded_pattern = [inverse_token_dict[int(i)] for i in items]
            result_patterns.append((decoded_pattern, support))

    os.remove(dataset_name)
    os.remove(output_name)

    return result_patterns


if __name__ == "__main__":
    # To generate the data
    import data_generators

    th = data_generators.GeneratedTheory([[1, -5, 4], [-1, 5, 3, 4], [-3, -10]])
    gen = data_generators.TheoryNoisyGeneratorOnDataset(th, 100, 1, 0.1)
    pos, neg = gen.generate_dataset()

    # pos = [frozenset{1, 2, -3,...}, frozenset{......}.......] but list instead of frozenset works (any iterable)

    # The part where
    support = 0.1
    frequent_patterns = compute_itemsets(pos, support, "Eclat")

    closed_patterns = compute_itemsets(pos, support, "LCM") # default one

    maximal_patterns = compute_itemsets(pos, support, "FPMax")

    print("Number of frequent itemsets: {}\n".format(len(frequent_patterns)))
    print("Number of closed frequent itemsets: {}\n".format(len(closed_patterns)))
    print("Number of maximal frequent itemsets: {}\n".format(len(maximal_patterns)))

    print("Closed patterns in decreased frequency order: {}".format(sorted(closed_patterns, key=lambda p: p[1], reverse=True)))

