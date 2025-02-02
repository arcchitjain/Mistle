# This file contains a wrapper to use the slim algorithm
# It mostly does dataset conversion and translation of slim tokens to initial tokens
from pathlib import Path
import configparser
from itertools import chain
import subprocess
import re
import logging
import shutil
import uuid
import os
import sys
import pickle


class Slim:
    def __init__(
        self,
        slim_exec_path,
        candidates="all",
        candidate_order="d",
        item2str_dict=None,
        datatype="bm128",
        nb_threads=1,
    ):
        """
        Creates a slim algorithm instance
        :param slim_exec_path: Path to the slim executable
        :param candidates: Candidates to consider : "all", "closed" or "cls" (don't know what this one is, check the paper?)
        :param candidate_order: Candidate set order determined by [ a (supp desc, length asc, lex) | d (like a, but length desc) | z | aq | as ... see the code ]
        :param item2str_dict: Dictionary to translate items into strings. Only used when producing readable_final_ct.txt file
        :param datatype: Preferred datatype ( uint16 | bai32 | bm128 (default) ). Always keep to default value (bm128) when the number of different items is <= 128.
        If the number of different items ('alphabet size') is > 128:
        Dense data --> choose bai32
        Sparse data --> choose uint16
        (You may do some small tests to find out what gives the best result for you with respect to both the required computational power and memory space.)
        :param nb_threads: Number of threads used by Slim
        """
        self.min_sup = 0

        self.slim_exec_path = slim_exec_path

        slim_home_dir = Path(self.slim_exec_path).parent
        self.slim_home_dir = f"{slim_home_dir}/"

        # Put all other parameters in here as well!
        self.code_table = []

        self.candidates = candidates
        self.candidate_order = candidate_order

        self.item2str_dict = item2str_dict
        self.datatype = datatype
        self.num_threads = nb_threads

    def compute_code_table(
        self,
        database_path,
        output_dir,
        min_support=1,
        convert_db=True,
        log=False,
        report=False,
    ):
        """
        Computes the code table using slim.
        :param database_path: Path to the database. Can be in SLim format directly (in that case disable convert_db).
        Can also be in a more classic format: each line is an itemset, and each itemset is a string of space separated numbers (in that case, set convert_db to true (default))
        :param output_dir: Directory where to output the results generated by Slim, plus the readable final code table file
        :param min_support: Minimal support for itemsets to be included in the code table
        :param convert_db: Whether to convert database to Slim format. See database path for details.
        :param log: Whether to output the log generated by Slim
        :param report: Whether to output the report generated by Slim
        :return: A code table as a list of elements. Each element is a tuple with first information being the support of the itemset and the second information being the itemset itself.
        """
        Path(output_dir).mkdir(exist_ok=True)
        logging.basicConfig(filename=str(Path(output_dir).joinpath("slim_wrapper.log")))

        # Use absolute path, as requested by SLIM doc
        database_path = os.path.abspath(database_path)
        output_dir = os.path.abspath(output_dir)

        data_dir = f"{Path(database_path).parent}/"

        # We first setup all the configuration files. Some might not be necessary
        datadir_conf_path = Path(self.slim_home_dir).joinpath("datadir.conf")
        config_datadir = read_config(datadir_conf_path)
        config_datadir["main"]["dataDir"] = data_dir
        config_datadir["main"]["expDir"] = f"{output_dir}/"
        save_config(config_datadir, datadir_conf_path)

        fic_conf_path = Path(self.slim_home_dir).joinpath("fic.conf")
        config_fic = read_config(fic_conf_path)
        config_fic["main"]["datadir"] = data_dir
        save_config(config_fic, fic_conf_path)

        user_fic_conf_path = Path(self.slim_home_dir).joinpath("fic.user.conf")
        config_fic_user = read_config(user_fic_conf_path)
        config_fic_user["main"]["datadir"] = data_dir
        config_fic_user["main"]["basedir"] = f"{output_dir}/"
        config_fic_user["main"]["xpsdir"] = f"{output_dir}/xps/"
        save_config(config_fic_user, user_fic_conf_path)

        if convert_db:
            if Path(database_path).suffix != ".dat":
                logging.info(
                    "Database "
                    + str(database_path)
                    + "not ending in .dat, creating temporary .dat copy"
                )
                shutil.copy(database_path, Path(database_path).with_suffix(".dat"))
                database_path = Path(database_path).with_suffix(".dat")

            # We convert the db
            convert_db_template_path = Path(self.slim_home_dir).joinpath(
                "convertdb.conf"
            )
            config_convert_db = read_config(convert_db_template_path)
            config_convert_db["main"]["dataDir"] = data_dir
            config_convert_db["main"]["dbName"] = Path(database_path).stem
            config_convert_db["main"]["dbInExt"] = Path(database_path).suffix[1:]
            config_convert_db_path = str(uuid.uuid4()) + "_convertdb.conf"
            save_config(config_convert_db, config_convert_db_path)

            subprocess.run([self.slim_exec_path, config_convert_db_path])
            os.remove(config_convert_db_path)
            database_path = Path(database_path).with_suffix(
                ".db"
            )  # The database is now in .db and is stored where the original one is

        # We check that database ends with .db
        if Path(database_path).suffix != ".db":
            logging.info(
                "Database "
                + str(database_path)
                + "not ending in .db, creating temporary .db copy"
            )
            shutil.copy(database_path, Path(database_path).with_suffix(".db"))
            database_path = Path(database_path).with_suffix(".db")

        # We now analyze the db to get the original items
        analysedb_template_path = Path(self.slim_home_dir).joinpath("analysedb.conf")
        analysedb_db = read_config(analysedb_template_path)
        analysedb_db["main"]["datadir"] = data_dir
        analysedb_db["main"]["dbName"] = Path(database_path).stem
        config_analysedb_path = str(uuid.uuid4()) + "_analysedb.conf"
        save_config(analysedb_db, config_analysedb_path)

        subprocess.run([self.slim_exec_path, config_analysedb_path])
        # Clean up
        os.remove(config_analysedb_path)

        analysis_result_path = Path(database_path).with_suffix(".db.analysis.txt")
        slim_item_dict = get_item_dictionary(analysis_result_path)

        # Finally, we compress!
        compress_template_path = Path(self.slim_home_dir).joinpath("compress.conf")
        compress_db = read_config(compress_template_path)
        compress_db["main"]["dataDir"] = data_dir
        compress_db["main"]["datatype"] = self.datatype
        compress_db["main"]["numThreads"] = str(self.num_threads)
        compress_db["main"]["iscName"] = (
            Path(database_path).stem
            + "-"
            + self.candidates
            + "-"
            + str(min_support)
            + self.candidate_order
        )
        compress_db["main"]["writeLogFile"] = "yes" if log else "no"
        compress_db["main"]["writeReportFile"] = "yes" if report else "no"
        config_compress_path = str(uuid.uuid4()) + "_compress.conf"
        save_config(compress_db, config_compress_path)

        subprocess.run([self.slim_exec_path, config_compress_path])
        os.remove(config_compress_path)
        Path(data_dir).joinpath("candidates").rmdir()
        Path(data_dir).joinpath("codetables").rmdir()
        Path(data_dir).joinpath("datasets").rmdir()

        # We get the latest directory created in the xps dir. This is not perfect but it should be fine. I did not find a way to get the directory name automatically
        base_res_dir = os.path.join(config_fic_user["main"]["xpsdir"], "compress_ng")
        dirs = [
            os.path.join(base_res_dir, d)
            for d in os.listdir(base_res_dir)
            if os.path.isdir(os.path.join(base_res_dir, d))
        ]
        res_dir = sorted(dirs, key=lambda x: os.path.getctime(x), reverse=True)[0]

        # We also get the last code table like this
        ct_files = [
            os.path.join(res_dir, f) for f in os.listdir(res_dir) if f.endswith(".ct")
        ]
        res_file = sorted(ct_files, key=lambda x: os.path.getctime(x), reverse=True)[0]

        self.code_table = []
        with open(res_file) as f:
            nb_line = 0
            regex = re.compile("(\d+)\s+")
            regex_support = re.compile("(\d+)\)")
            for line in f:
                nb_line += 1
                if nb_line > 2:
                    result = regex.findall(line)
                    if result:
                        # We only output patterns with more than 2 items
                        if len(result) > 1:
                            sup = regex_support.findall(line)[0]
                            self.code_table.append(
                                (sup, sorted([slim_item_dict[int(i)] for i in result]))
                            )

        with open(os.path.join(res_dir, "readable_final_ct.txt"), "w") as f_res:
            self.code_table.sort(key=lambda e: int(e[0]), reverse=True)
            for sup, p in self.code_table:
                if self.item2str_dict:
                    f_res.write(
                        " ".join([str(self.item2str_dict[i]) for i in p])
                        + " #SUP: "
                        + str(sup)
                        + "\n"
                    )
                else:
                    f_res.write(
                        " ".join([str(i) for i in p]) + " #SUP: " + str(sup) + "\n"
                    )

        return self.code_table


def read_config(f):
    """
    Reads the configuration file f in the SLIM format and "convert" it to standard python config
    :param f: path of the configuration file (str)
    :return: The configuration object
    """
    config = configparser.ConfigParser()
    config.optionxform = str  # Option to be case-sensitive
    with open(f) as lines:
        lines = chain(
            ("[main]",), lines
        )  # This line artificially adds a section to use the usual python parser
        config_lines = []
        for l in lines:
            if l.strip() == "EndConfig":
                break
            config_lines.append(l)
        config.read_file(config_lines)
    return config


def save_config(c, f):
    """
    Save python config c into file f. The resulting file is in the correct SLIM format
    :param c: the config to save
    :param f: The file path where config will be saved (str)
    :return:
    """
    with open(f, "w") as conf_file:
        c.write(conf_file)
        conf_file.write("EndConfig\n")

    with open(f) as conf_file:
        data = conf_file.read().splitlines(True)
    with open(f, "w") as conf_file:
        conf_file.writelines(data[1:])


def get_item_dictionary(analysis_result_path):
    """
    Returns the dictionary to convert SLIM item back to the original items
    :param analysis_result_path: The path (str) of the analysedb result
    :return:
    """
    slim_alphabet = {}
    with open(analysis_result_path) as f:
        alphabet_read = False
        for line in f:
            if "* Row lengths" in line:
                alphabet_read = False
            if alphabet_read:
                columns = line.split()
                if len(columns) > 0:
                    alphabet_entry = columns[0].split("=>")
                    if len(alphabet_entry) == 2:
                        slim_alphabet[int(alphabet_entry[0])] = int(alphabet_entry[1])

            if "* Alphabet" in line:
                alphabet_read = True
    return slim_alphabet


if __name__ == "__main__":
    db_name = sys.argv[1]
    min_support = int(sys.argv[2])
    if len(sys.argv) > 3:
        db_suffix = sys.argv[3]
        output_pickle = (
            "slim_" + sys.argv[1] + "_" + sys.argv[3] + "_" + sys.argv[2] + ".pckl"
        )
    else:
        db_suffix = None
        output_pickle = "slim_" + sys.argv[1] + "_" + sys.argv[2] + ".pckl"

    # db_name = "wff_3_100_150_100_100_20_data"
    # db_name = "ticTacToe"
    # db_suffix = "0_test_pos"
    # min_support = 50
    if db_suffix == None:
        db_file = "/home/arcchitjain/Documents/Mistle/Data/" + db_name + ".dat"
    else:
        db_file = (
            "/home/arcchitjain/Documents/Mistle/Output/"
            + db_name
            + "/"
            + db_name
            + "_"
            + db_suffix
            + ".dat"
        )

    slim_exec_path = "/home/arcchitjain/Documents/Mistle/Slim/bin/fic"
    output_dir = "/home/arcchitjain/Documents/Mistle/Output/"
    test_slim = Slim(slim_exec_path)
    code_table = test_slim.compute_code_table(
        db_file, output_dir, min_support=min_support, convert_db=True
    )

    with open(output_dir + output_pickle, "wb") as f:
        pickle.dump(code_table, f)

    print(code_table)
