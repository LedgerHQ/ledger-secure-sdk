#!/usr/bin/env python3
# coding: utf-8
"""
@BANNER@
"""
# -----------------------------------------------------------------------------
from sys import stderr
from os import listdir, makedirs, mkdir, path, remove
from json import load, dump
from shutil import rmtree, copyfile, copy2
from subprocess import run, CalledProcessError
from argparse import ArgumentParser

def run_command(command, timeout=None):
    """
    Run a command and return the generated output and an error flag.
    command is an array with command and args, like: ['ls', '-l']

    Return stdout+stderr concatenated, and error=False if no error occurred
    """
    try:
        # Avoid shell injection by using parameters in a string array.
        # subprocess will take care of adding necessary escape characters
        # around provided arguments.
        completed_process = run(command,
                                capture_output=True,
                                timeout=timeout,
                                check=True)

        # Everything went fine, return value is 0
        if completed_process.stdout:
            output = completed_process.stdout.decode("utf-8", 'ignore')
            output += completed_process.stderr.decode("utf-8", 'ignore')
        else:
            output = ""
        err = False

    # Do not handle subprocess.TimeoutExpired, voluntarily.
    # If a timeout is raised, check logs!
    # except subprocess.TimeoutExpired as error:
    #    # Timeout expired while waiting for the child process
    #    output = f"A timeout error occurred while sending command {command}: {error}"
    #    err = True

    except CalledProcessError as error:
        # This exception is called when returncode !=0
        output = f"Return code {error.returncode} when sending command {command}: {error}"
        output += '\nstderr = \n' + error.stderr.decode("utf-8", 'ignore')
        output += '\nstdout = \n' + error.stdout.decode("utf-8", 'ignore')
        err = True

    return output, err


# -----------------------------------------------------------------------------
def build_png(bin_file, json_file, work_dir, debug_file, properties_file,
              product_name, verbose=False):
    """
    Will run the build_png program with json_file and saves PNGs in work_dir.
    The output.txt file will be created in work_dir too.
    """
    # Call the program build_png with the JSON file provided by email
    cmd = [bin_file, '-d', work_dir, '-p', properties_file,
           '-n', product_name]
    if verbose:
        cmd += ['-v']
    if json_file:
        cmd += ['-j', json_file]

    output, error = run_command(cmd, timeout=60.0)
    if error:
        # An error occurred: log all details
        print(output)
        return None

    # Save the output to output.txt file
    debug_file.write(output)

    return output


# -----------------------------------------------------------------------------
def get_existing_flows(flow_dir):
    """
    get all existing flows from input flows directory.
    """
    # first level is the type of flow (os or app-sdk)
    type_dirs = listdir(flow_dir)
    existing_flows = {}
    for type_dir in type_dirs:
        if path.isdir(path.join(flow_dir, type_dir)):
            dirs = listdir(path.join(flow_dir, type_dir))
            existing_screenshot_flows = []
            for file in dirs:
                if path.isfile(path.join(flow_dir, type_dir, file)) and \
                                        file.endswith('.json') and \
                                        file.startswith('flow_'):
                    flow_name = file.replace('flow_', '').replace('.json', '')
                    if flow_name != 'unused':
                        existing_screenshot_flows.append(flow_name)
            existing_flows[type_dir] = sorted(existing_screenshot_flows)

    return existing_flows


# -----------------------------------------------------------------------------
def get_displayable_string(text):
    """
    Replace specific characters by corresponding displayable ones.
    """
    text = text.replace('\x08', "\\b")
    text = text.replace('\n', "\\n")
    text = text.replace('\f', "\\f")
    return text


# -----------------------------------------------------------------------------
def screenshots2html(build_dir, flow, product_name, all_errors):
    """
    This function creates the screenshots HTML file for one flow, using
    the input json file for this flow
    """
    input_json_file = path.join(build_dir, flow+'_flow.json')
    output_html_file = path.join(build_dir, flow+'_flow.html')

    # Can't generate screenshots for unused IDs, for now...
    if flow == "unused":
        return all_errors

    # Check input file does exists
    if not path.isfile(input_json_file):
        stderr.write(f"Can't find input file {input_json_file}!\n")
        return all_errors

    infile = open(input_json_file, "r", encoding="utf-8")
    data = load(infile)

    # do not create html file if no page in json file
    if len(data['pages']) == 0:
        infile.close()
        return all_errors

    outfile = open(output_html_file, "w", encoding="utf-8")
    outfile.write("<!DOCTYPE html>\n")
    outfile.write("<html>\n")
    outfile.write("<head>\n")
    outfile.write("<style>\n")
    outfile.write("table, th, td {\n")
    outfile.write("  border: 1px solid black;\n")
    outfile.write("  border-collapse: collapse;\n")
    outfile.write("}\n")
    outfile.write("</style>\n")
    outfile.write(f"  <title>{data['name']} flow</title>\n")
    outfile.write("</head>\n")
    outfile.write("<meta charset=\"utf-8\">\n")
    outfile.write("<body>\n")
    outfile.write(f"  <h1>{data['name']} flow</h1>\n")
    outfile.write("  <table>\n")
    outfile.write("    <tr>\n")
    outfile.write("      <th>Name</th>\n")
    outfile.write("      <th>Screen</th>\n")
    outfile.write("    </tr>\n")
    for page in data['pages']:
        if 'Entry' in page['name']:
            continue
        outfile.write("    <tr>\n")
        outfile.write(f"      <td>{page['name']} </td>\n")
        outfile.write("      <td id=\"{0}\"> <img src=\"{1}\" width=\"300\"/></td>\n".format(page['name'], page['image']))
        outfile.write("    </tr>\n")
    outfile.write("  </table>\n")
    outfile.write("</body>\n")
    outfile.write("</html>\n")

    outfile.close()
    infile.close()

    return all_errors

def create_index_file(index_html, dir_flows):
    outfile = open(index_html, "w", encoding="utf-8")
    outfile.write('<!DOCTYPE html>\n')
    outfile.write('<html>\n')
    outfile.write('<head>\n')
    outfile.write('    <title>Index page for all flows</title>\n')
    outfile.write('</head>\n')
    outfile.write('<meta charset=\"utf-8\">\n')
    outfile.write('<body bgcolor=\"#dddddd\">\n')
    outfile.write('   <h1>Index page for all flows<h1>\n')
    for flow_type, flows in dir_flows.items():
        outfile.write(f'   <h2>{flow_type}<h2>\n')
        outfile.write('   <p><h4><br>\n')
        for flow in flows:
            outfile.write(f'      <a href=\"{flow_type}/{flow}_flow.html\">{flow}</a><br>\n')
            outfile.write('      <br>\n')
        outfile.write('    </h4></p>\n')
    outfile.write('  </body>\n')
    outfile.write('</html>\n')

    outfile.close()

def main(binary, build_dir, flow_dir, properties_file,
         product_name, verbose) -> int:
    """
    Main function
    """
    # main index file to produce
    index_html = path.join(build_dir, 'index.html')

    # debug file to produce
    output_txt = path.join(build_dir, 'output.txt')

    # delete whole directory and recreate it
    rmtree(build_dir, ignore_errors=True)
    makedirs(build_dir)

    # delete debug file
    if path.exists(output_txt):
        remove(output_txt)

    debug_file = open(output_txt, "w", encoding="utf-8")
    # get all existing flows from input flows directories
    existing_screenshot_flows = get_existing_flows(flow_dir)

    all_errors = []
    # how to generate screenshots for a given scenario (flow): it produces the <flow>.html
    for flow_type, flows in existing_screenshot_flows.items():
        type_build_dir = path.join(build_dir, flow_type)
        mkdir(type_build_dir)
        for flow in flows:
            if verbose:
                debug_file.write(f'\nGenerating screenshots for ... {flow_type}/{flow}\n\n')
            mkdir(path.join(type_build_dir, flow))
            output = build_png(binary,
                               path.join(flow_dir, flow_type, 'flow_'+flow+'.json'),
                               type_build_dir,
                               debug_file,
                               properties_file,
                               product_name,
                               verbose)
            if output is None:
                return -1
            all_errors = screenshots2html(
                type_build_dir, flow, product_name, all_errors)

    # clean-up non existing flows (because not concerned for this product)
    for flow_type, flows in existing_screenshot_flows.items():
        for flow in flows:
            if not path.exists(path.join(build_dir, flow_type, flow+'_flow.html')):
                existing_screenshot_flows[flow_type].remove(flow)

    if verbose:
        debug_file.write('Generating index file...\n')
    create_index_file(index_html, existing_screenshot_flows)

    if verbose:
        debug_file.write('\n')

    # Display errors
    for error_msg in all_errors:
        debug_file.write(error_msg)

    if verbose:
        debug_file.write('\n')

    if len(all_errors) != 0:
        debug_file.write(f"{len(all_errors)} error(s) found.\n")
    else:
        debug_file.write("No error found, congratulations!\n")

    debug_file.close()

    return 0


# -------------------------------------------------------------------------
# Program entry point:
# -----------------------------------------------------------------------------
if __name__ == "__main__":
    # -------------------------------------------------------------------------
    # Parse arguments:
    parser = ArgumentParser(
        description="generate screenshots for all flows"
        "in the given work dir")

    parser.add_argument(
        "-b", "--binary",
        dest="binary", type=str,
        help="binary file")

    parser.add_argument(
        "-f", "--flow_dir",
        dest="flow_dir", type=str,
        required=True,
        help="directory where to find flow Jsons")

    parser.add_argument(
        "-w", "--work_dir",
        dest="work_dir", type=str,
        required=True,
        help="working directory")

    parser.add_argument(
        "-p", "--properties_file",
        dest="properties_file", type=str,
        help="<target>.properties file path")

    parser.add_argument(
        "-n", "--product_name",
        dest="product_name", type=str,
        help="product name (stax, nanox, flex...)")

    parser.add_argument(
        "-v", "--verbose",
        action='store_true',
        help="Add verbosity to output ('%(default)s' by default)")

    args = parser.parse_args()

    # -------------------------------------------------------------------------
    # Call main function:
    EXIT_CODE = main(args.binary, args.work_dir,
                         args.flow_dir, args.properties_file, args.product_name, args.verbose)

    exit(EXIT_CODE)
