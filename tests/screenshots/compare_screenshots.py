import math
import os
import argparse
import sys
import hashlib
import json
from PIL import Image as img, ImageOps
from PIL.Image import Image
from typing import Tuple, Optional
import shutil

def get_png_files(directory):
    # Check if the JSON file already exists and print its content
    if os.path.isfile(os.path.join(directory, 'hashes.json')):
        with open(os.path.join(directory, 'hashes.json'), 'r') as json_file:
            existing_data = json.load(json_file)
            return existing_data

    all_png_files = {}
    """Recursively get all <flow>_flow.json files in a directory."""
    if not os.path.exists(directory):
        return all_png_files
    dirs = os.listdir(directory)
    for dir in dirs:
        if os.path.isdir(os.path.join(directory, dir)):
            all_png_files[dir] = {}
            files = os.listdir(os.path.join(directory, dir))
            for file in files:
                if file.lower().endswith('_flow.json'):
                    png_files = {}
                    file_path = os.path.join(directory, dir, file)
                    with open(file_path, 'r') as json_file:
                        existing_data = json.load(json_file)
                        pages = existing_data["pages"]
                        for page in pages:
                            file_hash = page["hash"]
                            relative_path = os.path.basename(page["image"])
                            png_files[relative_path] = file_hash
                    all_png_files[dir][existing_data["name"]] = png_files
    return all_png_files

def compare_file_trees(ref, target):
    """Compare two directories of PNG files."""
    ref_files = get_png_files(ref)
    target_files = get_png_files(target)

    differences = {
        'only_in_ref': [],
        'only_in_target': [],
        'different_hash': []
    }
    if len(ref_files) == 0:
        print('No reference for this comparison')
        sys.exit(-1)
    for dir, values in ref_files.items():
        target_dir_keys = target_files[dir].keys()
        for flow, pages in values.items():
            if flow not in target_dir_keys:
                differences['only_in_ref'].append(os.path.join(dir,flow))
            else:
                for page, hash in pages.items():
                    if page not in target_files[dir][flow].keys():
                        differences['only_in_ref'].append(os.path.join(dir,flow,page))
                    elif hash != target_files[dir][flow][page]:
                        differences['different_hash'].append(os.path.join(dir,flow,page))
        for target_dir_flow, target_dir_pages in target_files[dir].items():
            if target_dir_flow not in ref_files[dir].keys():
                differences['only_in_target'].append(os.path.join(dir,target_dir_flow))
            else:
                for page in target_dir_pages.keys():
                    if page not in ref_files[dir][target_dir_flow].keys():
                        differences['only_in_target'].append(os.path.join(dir,target_dir_flow,page))

    if len(differences['only_in_ref']) == 0 and len(differences['only_in_target']) == 0 and len(differences['different_hash']) == 0:
        return None
    return differences

def print_differences(differences):
    """Print the differences between two file trees."""
    if len(differences['only_in_ref']) > 0:
        print("Files missing in target:")
        for file in sorted(differences['only_in_ref']):
            print(f"  {file}")

    if len(differences['only_in_target']) > 0:
        print("\nFiles missing in reference:")
        for file in sorted(differences['only_in_target']):
            print(f"  {file}")

    if len(differences['different_hash']) > 0:
        print("\nFiles with different hashes:")
        for file in sorted(differences['different_hash']):
            print(f"  {file}")

def save_golden(ref_dir, target_dir):
    # Save hashes to a JSON file
    target_files = get_png_files(target_dir)

    if not os.path.exists(ref_dir):
        os.mkdir(ref_dir)
    with open(os.path.join(ref_dir,"hashes.json"), 'w') as json_file:
        json.dump(target_files, json_file, indent=4)

    for dir, values in target_files.items():
        if os.path.exists(os.path.join(ref_dir, dir)):
            shutil.rmtree(os.path.join(ref_dir, dir))
        os.mkdir(os.path.join(ref_dir, dir))
        for flow, pages in values.items():
            os.mkdir(os.path.join(ref_dir, dir, flow))
            for key in pages.keys():
                if not (key.startswith('Entry') and values[flow][key] == ''):
                    shutil.copy(os.path.join(target_dir, dir, flow, key), os.path.join(ref_dir, dir, flow, key))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generates comparisons (and differences) between two sets of images, the first one being the <reference> (golden_samples) "
        "and the other the <target>.")

    parser.add_argument(
        "-r", "--reference",
        dest="reference", type=str,
        required=True,
        help="path to the reference set of images")

    parser.add_argument(
        "-t", "--target",
        dest="target", type=str,
        required=True,
        help="path to the target set of images, to compare with reference")

    parser.add_argument(
        "-g", "--golden",
        action='store_true',
        help="if set, means that the target will replace the reference")

    args = parser.parse_args()
    if not args.golden:
        differences = compare_file_trees(args.reference, args.target)
    else:
        save_golden(args.reference, args.target)
        differences = None
    if differences is not None:
        print_differences(differences)
        sys.exit(-1)
