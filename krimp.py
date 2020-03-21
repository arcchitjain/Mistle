from pathlib import Path
import configparser
from itertools import chain
import subprocess
import re
from krimp_wrapper import Krimp

# We first convert the original dataset into krimp format
def read_config(f):
    config = configparser.ConfigParser()
    config.optionxform = str
    with open(f) as lines:
        lines = chain(("[main]",), lines)  # This line does the trick.
        config_lines = []
        for l in lines:
            if l.strip() == "EndConfig":
                break
            config_lines.append(l)
        config.read_file(config_lines)
    return config


def run_KRIMP():
    convert_db_path = Path(krimp_home_dir).joinpath("convertdb.conf")
    config_convert_db = read_config(convert_db_path)
    config_convert_db["main"]["dataDir"] = f"{Path(db_file).parent}/"
    config_convert_db["main"]["dbName"] = Path(db_file).stem
    config_convert_db["main"]["dbInExt"] = Path(db_file).suffix[1:]
    with open(convert_db_path.with_suffix(".test"), "w") as conf_file:
        config_convert_db.write(conf_file)
        conf_file.write("EndConfig\n")

    with open(convert_db_path.with_suffix(".test")) as conf_file:
        data = conf_file.read().splitlines(True)
    with open(convert_db_path.with_suffix(".test"), "w") as conf_file:
        conf_file.writelines(data[1:])

    datadir_conf_path = Path(krimp_home_dir).joinpath("datadir.conf")
    config_datadir = read_config(datadir_conf_path)
    config_datadir["main"]["dataDir"] = str(Path(db_file).parent) + "/"
    config_datadir["main"]["expDir"] = (
        str(Path(db_file).parent.parent.joinpath("Output")) + "/"
    )
    with open(datadir_conf_path, "w") as conf_file:
        config_datadir.write(conf_file)
        conf_file.write("EndConfig\n")

    with open(datadir_conf_path) as conf_file:
        data = conf_file.read().splitlines(True)
    with open(datadir_conf_path, "w") as conf_file:
        conf_file.writelines(data[1:])

    fic_conf_path = Path(krimp_home_dir).joinpath("fic.conf")
    config_fic = read_config(fic_conf_path)
    config_fic["main"]["datadir"] = str(Path(db_file).parent) + "/"
    with open(fic_conf_path, "w") as conf_file:
        config_fic.write(conf_file)
        conf_file.write("EndConfig\n")

    with open(fic_conf_path) as conf_file:
        data = conf_file.read().splitlines(True)
    with open(fic_conf_path, "w") as conf_file:
        conf_file.writelines(data[1:])

    user_fic_conf_path = Path(krimp_home_dir).joinpath("fic.user.conf")
    config_fic_user = read_config(user_fic_conf_path)
    config_fic_user["main"]["datadir"] = str(Path(db_file).parent) + "/"
    config_fic_user["main"]["basedir"] = (
        str(Path(db_file).parent.parent.joinpath("Output")) + "/"
    )
    config_fic_user["main"]["xpsdir"] = (
        str(Path(db_file).parent.parent.joinpath("Output/xps")) + "/"
    )
    with open(user_fic_conf_path, "w") as conf_file:
        config_fic_user.write(conf_file)
        conf_file.write("EndConfig\n")

    with open(user_fic_conf_path) as conf_file:
        data = conf_file.read().splitlines(True)
    with open(user_fic_conf_path, "w") as conf_file:
        conf_file.writelines(data[1:])

    print(krimp_exec_path + " " + str(convert_db_path.with_suffix(".test")))
    subprocess.run([krimp_exec_path, str(convert_db_path.with_suffix(".test"))])

    # We now analyze the db
    analysedb_path = Path(krimp_home_dir).joinpath("analysedb.conf")
    analysedb_db = read_config(analysedb_path)
    analysedb_db["main"]["datadir"] = str(Path(db_file).parent) + "/"
    analysedb_db["main"]["dbName"] = Path(db_file).stem
    print("Writing\t:" + str(analysedb_path.with_suffix(".test")))
    with open(analysedb_path.with_suffix(".test"), "w") as conf_file:
        analysedb_db.write(conf_file)
        conf_file.write("EndConfig\n")

    with open(analysedb_path.with_suffix(".test")) as conf_file:
        data = conf_file.read().splitlines(True)
    with open(analysedb_path.with_suffix(".test"), "w") as conf_file:
        conf_file.writelines(data[1:])

    print(krimp_exec_path + " " + str(analysedb_path.with_suffix(".test")))
    subprocess.run([krimp_exec_path, str(analysedb_path.with_suffix(".test"))])

    # Finally, we compress!
    compress_path = Path(krimp_home_dir).joinpath("compress.conf")
    compress_db = read_config(compress_path)
    compress_db["main"]["dataDir"] = str(Path(db_file).parent) + "/"
    compress_db["main"]["iscName"] = Path(db_file).stem + "-all-" + min_support + "d"
    with open(compress_path.with_suffix(".test"), "w") as conf_file:
        compress_db.write(conf_file)
        conf_file.write("EndConfig\n")

    with open(compress_path.with_suffix(".test")) as conf_file:
        data = conf_file.read().splitlines(True)
    with open(compress_path.with_suffix(".test"), "w") as conf_file:
        conf_file.writelines(data[1:])

    print(krimp_exec_path + " " + str(compress_path.with_suffix(".test")))
    subprocess.run([krimp_exec_path, str(compress_path.with_suffix(".test"))])


def get_code_table(output_path):
    # We now try to use KRIMP on this data
    # We first look at the db alphabet generated by the krimp convertdb tool
    krimp_alphabet = {}
    stat_file = str(Path(db_file).parent.joinpath(db_name + ".db.analysis.txt"))
    with open(stat_file) as f:
        alphabet_read = False
        for line in f:
            if "* Row lengths" in line:
                alphabet_read = False
            if alphabet_read:
                columns = line.split()
                if len(columns) > 0:
                    alphabet_entry = columns[0].split("=>")
                    if len(alphabet_entry) == 2:
                        krimp_alphabet[int(alphabet_entry[0])] = int(alphabet_entry[1])

            if "* Alphabet" in line:
                alphabet_read = True

    ct_file = Path(db_file).parent.parent.joinpath("Output/xps/compress/" + output_path)
    res_patterns = []
    with open(ct_file) as f:
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
                        res_patterns.append(
                            (sup, [krimp_alphabet[int(i)] for i in result])
                        )

    with open("Output/result_krimp_" + db_name + ".out", "w") as f_res:
        res_patterns.sort(key=lambda e: int(e[0]), reverse=True)
        for sup, p in res_patterns:
            f_res.write(" ".join([str(i) for i in p]) + " #SUP: " + str(sup) + "\n")


min_support = "1"
db_name = "ticTacToe"
db_file = "/home/dtai/PycharmProjects/Mistle/Data/" + db_name + ".dat"
krimp_exec_path = "/home/dtai/PycharmProjects/Mistle/Krimp/bin/krimp"
krimp_home_dir = Path(krimp_exec_path).parent
krimp_home_dir = f"{krimp_home_dir}/"
output_dir = "/home/dtai/PycharmProjects/Mistle/Output/"
Path(output_dir).mkdir(exist_ok=True)


run_KRIMP()
# get_code_table(
#     "ticTacToe-all-50d-pop-20200321173853/ct-ticTacToe-all-50d-pop-20200321173853-50-1362.ct"
# )


test_krimp = Krimp(krimp_exec_path)
code_table = test_krimp.compute_code_table(
    db_file, output_dir, min_support=100, convert_db=True
)
print(code_table)
