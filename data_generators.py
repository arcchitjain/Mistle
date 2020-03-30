from abc import ABC, abstractmethod
import pycosat
import numpy as np
from sympy.core import Symbol
from sympy.logic.boolalg import And, Or, Not, to_cnf
import random
from copy import copy


class GeneratedTheory:
    def __init__(self, clauses):
        """
        Initiates a Generated theory as a CNF of clauses
        :param clauses: list of clauses as a DIMACS like format. Example: [[1, -2, 3], [-5, -4]] represents the theory (x1 or not(x2) or x3) and (not(x5) or not(x4))
        """
        self.clauses = clauses
        self.nb_literals = max([abs(l) for c in self.clauses for l in c])

    @staticmethod
    def from_string(string_repr):
        """
        Converts a string of a CNF into a list of clauses
        Symbols are replaced by numbers in the order of their occurrences.
        Example: (a|~b|c)&(~d|~c) becomes the list [[1, -2, 3], [-4, -3]]
        :param string_repr:
        :return:
        """
        # TODO
        pass

    def get_negated_theory(self):
        """
        Negates a theory
        :return: A new generated theory, corresponding to the negation of the current theory
        """
        sympy_and_clauses = []
        for c in self.clauses:
            sympy_or_clauses = []
            for l in c:
                if l > 0:
                    # sympy_or_clauses.append(Symbol("cnf_%s" % l))
                    sympy_or_clauses.append(~Symbol("cnf_%s" % l))
                else:
                    # sympy_or_clauses.append(~Symbol("cnf_%s" % abs(l)))
                    sympy_or_clauses.append(Symbol("cnf_%s" % abs(l)))
            # sympy_and_clauses.append(Or(*sympy_or_clauses))
            sympy_and_clauses.append(And(*sympy_or_clauses))
        # sympy_cnf = And(*sympy_and_clauses)
        sympy_cnf = Or(*sympy_and_clauses)
        # negated_cnf = to_cnf(~sympy_cnf, True)
        negated_cnf = to_cnf(sympy_cnf, True)
        negated_cnf1 = to_cnf(sympy_cnf, False)
        negated_cnf_dimacs = []

        if isinstance(negated_cnf, And):
            for clause in negated_cnf.args:
                or_literals = []
                if isinstance(clause, Or):
                    for sympy_literal in clause.args:
                        or_literals.append(
                            TheoryNoisyGenerator.get_dimacs_repr(sympy_literal)
                        )
                else:
                    or_literals.append(TheoryNoisyGenerator.get_dimacs_repr(clause))
                negated_cnf_dimacs.append(or_literals)
        else:
            or_literals = []
            if isinstance(negated_cnf, Or):
                for sympy_literal in negated_cnf.args:
                    or_literals.append(
                        TheoryNoisyGenerator.get_dimacs_repr(sympy_literal)
                    )
            else:
                or_literals.append(TheoryNoisyGenerator.get_dimacs_repr(negated_cnf))
            negated_cnf_dimacs.append(or_literals)

        # negated_cnf_dimacs.append(
        #     [TheoryNoisyGenerator.get_dimacs_repr(negated_cnf)]
        # )
        return GeneratedTheory(negated_cnf_dimacs)


class DataGenerator(ABC):
    @abstractmethod
    def generate_dataset(self):
        pass

    def generate_datasets(self, n=1):
        return [self.generate_dataset() for i in range(n)]


class TheoryNoisyGenerator(DataGenerator):
    def __init__(
        self, theory: GeneratedTheory, nb_positives=100, nb_negatives=100, noise=0.1
    ):
        """
        Creates a new generator that generates noisy data satisfying (or not) a given theory.
        This means that for each example the proportion of noise is exactly equal to the noise parameter.
        There are nb_positives examples satisfying the theory and nb_negatives examples not satisfying the theory.
        Theory satisfaction is checked BEFORE noise is applied.
        :param theory: Instance of a GeneratedTheory
        :param nb_positives: Number of positive examples
        :param nb_negatives: Number of negative examples
        :param noise:
        """

        self.theory = theory
        self.nb_positives = nb_positives
        self.nb_negatives = nb_negatives
        self.noise = noise

    def generate_all_examples(self):
        """
        Generates the set of examples satisfying the theory and the set of of examples not satisfying it.
        The whole sets of examples are returned, so this is practical only for theories with a small number of literals (15 is reasonnable).
        :return: 2 list. The first one is the list of positive assignments, the second is the list of negative assignments
        """
        all_positive_examples = list(pycosat.itersolve(self.theory.clauses))

        if len(all_positive_examples) == 0:
            raise Exception("Theory is UNSAT. Impossible to generate the full dataset")

        all_negative_examples = list(
            pycosat.itersolve(self.theory.get_negated_theory().clauses)
        )
        if len(all_negative_examples) == 0:
            raise Exception(
                "Negated theory is UNSAT. Impossible to generate the full dataset"
            )

        assert (
            len(all_positive_examples) + len(all_negative_examples)
            == 2 ** self.theory.nb_literals
        )

        return all_positive_examples, all_negative_examples

    def generate_complete_examples(self, use_all_examples=True):
        """
        :param use_all_examples: If set to True, the set of al examples is first generated, and then samples are taken from it to form the whole dataset.
            This setting is doable for theories with few literals (about 15 literals).
            If False, samples are iteratively generated from the pycosat itersolve function until the dataset is complete
        :return:
        :param use_all_examples:
        :return:
        """
        positive = []
        negative = []
        if use_all_examples:
            all_pos, all_neg = self.generate_all_examples()
            positive_indices = np.random.choice(
                len(all_pos), self.nb_positives, replace=True
            ).tolist()
            negative_indices = np.random.choice(
                len(all_neg), self.nb_negatives, replace=True
            ).tolist()

            positive = [all_pos[i] for i in positive_indices]
            negative = [all_neg[i] for i in negative_indices]

        else:
            # We use the first nb_positive and nb_negative solutions in pycosat
            for sol in pycosat.itersolve(self.theory.clauses):
                positive.append(sol)
                if len(positive) == 10 * self.nb_positives:
                    break

            positive = random.sample(positive, self.nb_positives)

            # for sol in pycosat.itersolve(self.theory.get_negated_theory().clauses):
            #     negative.append(sol)
            #     if len(negative) == self.nb_negatives:
            #         break

            negative = []
            while True:
                clauses = copy(self.theory.clauses)
                neg = []
                for i in range(1, self.theory.nb_literals + 1):
                    # for i, j in enumerate(random.getrandbits(self.theory.nb_literals)):
                    #     if bool(j) is True:
                    if random.random() < 0.5:
                        clauses.append([i])
                        neg.append(i)
                    else:
                        clauses.append([-i])
                        neg.append(-i)

                if pycosat.solve(clauses) == "UNSAT":
                    negative.append(neg)

                if len(negative) == self.nb_negatives:
                    break

            # If some positive or negative examples are missing, we resample from all possible solutions
            if len(positive) < self.nb_positives:
                positive_indices = np.random.choice(
                    len(positive), self.nb_positives - len(positive), replace=True
                ).tolist()
                for i in positive_indices:
                    positive.append(positive[i])

            if len(negative) < self.nb_negatives:
                negative_indices = np.random.choice(
                    len(negative), self.nb_negatives - len(negative), replace=True
                ).tolist()
                for i in negative_indices:
                    negative.append(negative[i])

        return positive, negative

    @staticmethod
    def get_dimacs_repr(symbol):
        """
        Get dimacs representation of a single symbol
        :param symbol:
        :return:
        """
        if isinstance(symbol, Not):
            for l in symbol.args:
                return -1 * TheoryNoisyGenerator.get_dimacs_repr(l)
        else:
            literal = int(symbol.name.split("_")[1])
            if symbol.name.startswith("~"):
                literal = -1 * literal

            return literal


class TheoryNoisyGeneratorOnExample(TheoryNoisyGenerator):
    def __init__(self, theory, nb_positives=100, nb_negatives=100, noise=0.1):
        """
        Creates a new generator that adds noise at the example level.
        This means that for each example the proportion of noise is exactly equal to the noise parameter.
        There are nb_positives examples satisfying the theory and nb_negatives examples not satisfying the theory.
        Theory satisfaction is checked BEFORE noise is applied.
        :param theory: Instance of a GeneratedTheory
        :param nb_positives: Number of positive examples
        :param nb_negatives: Number of negative examples
        :param noise:
        """
        super().__init__(theory, nb_positives, nb_negatives, noise)

    def generate_dataset(self, use_all_examples=True):
        """
        Generates the dataset given corresponding to the created generator.
        :param use_all_examples: If True, will first generate all examples and sample from these. This can be impractical for theories with many literals (about 15 literals is ok).
        :return: 2 list. The first list contains positive examples in DIMACS format (as a frozenset), the second contains the negative examples
        """
        complete_pos, complete_neg = self.generate_complete_examples(
            use_all_examples=use_all_examples
        )
        partial_pos = []
        partial_neg = []

        # For each example, we add self.noise amount of noise
        for example in complete_pos:
            partial_pos.append(
                frozenset(
                    np.random.choice(
                        example, int((1 - self.noise) * len(example)), replace=False
                    ).tolist()
                )
            )

        for example in complete_neg:
            partial_neg.append(
                frozenset(
                    np.random.choice(
                        example, int((1 - self.noise) * len(example)), replace=False
                    ).tolist()
                )
            )

        return partial_pos, partial_neg


class TheoryNoisyGeneratorOnDataset(TheoryNoisyGenerator):
    def __init__(self, theory, nb_positives=100, nb_negatives=100, noise=0.1):
        """
        Creates a new generator that adds noise at the dataset level.
        This means that the noise level is applied on the whole dataset.
        Individual examples might therefore have a different level of noise.
        There are nb_positives examples satisfying the theory and nb_negatives examples not satisfying the theory.
        Theory satisfaction is checked BEFORE noise is applied.
        :param theory: Instance of a GeneratedTheory
        :param nb_positives: Number of positive examples
        :param nb_negatives: Number of negative examples
        :param noise:
        """
        super().__init__(theory, nb_positives, nb_negatives, noise)

    def generate_dataset(self, use_all_examples=True):
        """
        Generates the dataset given corresponding to the created generator.
        :param use_all_examples: If True, will first generate all examples and sample from these. This can be impractical for theories with many literals (about 15 literals is ok).
        :return: 2 list. The first list contains positive examples in DIMACS format (as a frozenset), the second contains the negative examples
        """
        complete_pos, complete_neg = self.generate_complete_examples(
            use_all_examples=use_all_examples
        )
        partial_pos = []
        partial_neg = []

        nb_pos_literals = sum([len(e) for e in complete_pos])
        nb_neg_literals = sum([len(e) for e in complete_neg])

        # We apply noise on the whole dataset, not at the transaction level by skipping random idices according to the noise level on the whole data
        skipped_indices = frozenset(
            np.random.choice(
                nb_pos_literals + nb_neg_literals,
                int(self.noise * (nb_pos_literals + nb_neg_literals)),
                replace=False,
            )
        )
        current_index = 0

        for example in complete_pos:
            partial_pos.append(
                frozenset(
                    [
                        example[i]
                        for i in range(len(example))
                        if current_index + i not in skipped_indices
                    ]
                )
            )
            current_index += len(example)

        for example in complete_neg:
            partial_neg.append(
                frozenset(
                    [
                        example[i]
                        for i in range(len(example))
                        if current_index + i not in skipped_indices
                    ]
                )
            )
            current_index += len(example)

        assert current_index == nb_neg_literals + nb_pos_literals

        return partial_pos, partial_neg


if __name__ == "__main__":
    # th = GeneratedTheory([[1, -5, 4], [-1, 5, 3, 4], [-3, -10]])
    cnf = []
    filename = "wff_3_100_150.cnf"
    with open("./Data/" + filename, "r") as cnf_file:
        lines = cnf_file.readlines()
        for line in lines[2:]:
            line = line.strip()
            cnf.append([int(number) for number in line.split(" ")[:-1]])

    th = GeneratedTheory(cnf)
    input_params = []
    input_params.append((100, 100, 0.2))
    input_params.append((100, 100, 0.4))
    input_params.append((100, 100, 0.6))
    input_params.append((100, 100, 0.8))
    input_params.append((500, 500, 0.2))
    input_params.append((500, 500, 0.4))
    input_params.append((1000, 1000, 0.2))
    input_params.append((1000, 1000, 0.4))

    for nb_positives, nb_negatives, noise in input_params:
        gen = TheoryNoisyGeneratorOnExample(th, nb_positives, nb_negatives, noise)
        pos, neg = gen.generate_dataset(use_all_examples=False)

        with open(
            "./Data/"
            + filename.split(".cnf")[0]
            + "_"
            + str(gen.nb_positives)
            + "_"
            + str(gen.nb_negatives)
            + "_"
            + str(int(gen.noise * 100))
            + "_ex.dat",
            "w+",
        ) as out_file:
            for p in pos:
                l = list(p)
                abs_l = [abs(i) for i in l]
                p = [str(x) for _, x in sorted(zip(abs_l, l))]

                out_file.write(" ".join(p) + " " + str(th.nb_literals + 1) + "\n")
            for n in neg:
                l = list(n)
                abs_l = [abs(i) for i in l]
                n = [str(x) for _, x in sorted(zip(abs_l, l))]

                out_file.write(" ".join(n) + " " + str(th.nb_literals + 2) + "\n")

        gen2 = TheoryNoisyGeneratorOnDataset(th, nb_positives, nb_negatives, noise)
        pos2, neg2 = gen2.generate_dataset(use_all_examples=False)

        with open(
            "./Data/"
            + filename.split(".cnf")[0]
            + "_"
            + str(gen2.nb_positives)
            + "_"
            + str(gen2.nb_negatives)
            + "_"
            + str(int(gen2.noise * 100))
            + "_data.dat",
            "w+",
        ) as out_file:
            for p in pos:
                l = list(p)
                abs_l = [abs(i) for i in l]
                p = [str(x) for _, x in sorted(zip(abs_l, l))]

                out_file.write(" ".join(p) + " " + str(th.nb_literals + 1) + "\n")
            for n in neg:
                l = list(n)
                abs_l = [abs(i) for i in l]
                n = [str(x) for _, x in sorted(zip(abs_l, l))]

                out_file.write(" ".join(n) + " " + str(th.nb_literals + 2) + "\n")
