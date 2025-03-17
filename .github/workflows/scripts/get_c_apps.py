from ledgered.github import GitHubLedgerHQ, NoManifestException, Condition
from github.GithubException import GithubException

import sys
import json

if len(sys.argv) != 2:
    print("Usage: get_rust_apps.py <github_token>")
    sys.exit(1)

# Excluded Rust apps
# app-kadena-legacy: has been replaced by app-kadena
# app-pocket: does not build (Obsidians' Alamgu issue)
excluded_apps = ["app-avalanche"]

# Retrieve all public apps on LedgerHQ GitHub organization
token = sys.argv[1]
gh = GitHubLedgerHQ(token)
apps=gh.apps.filter(private=Condition.WITHOUT, archived=Condition.WITHOUT)

c_apps = []
# loop all apps in gh.apps
for app in apps:
    try: 
        manifest = app.manifest
    except NoManifestException as e:
        pass
    except GithubException as e:
        pass
    else:
        # Filter out apps that are C based
        if manifest.app.sdk == "c":
            if app.name not in excluded_apps:
                c_apps.append({"app-name": app.name, "build-directory": str(manifest.app.build_directory) ,"devices": manifest.app.devices.json})

# Print the number of apps to build
print("Nb of apps to build: ", len(c_apps))

# save the list of apps to build in a json format:
with open("c_apps.json", "w") as f:
    f.write(json.dumps(c_apps))
