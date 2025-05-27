import sys
import json
from ledgered.github import GitHubLedgerHQ, NoManifestException, Condition  # type: ignore
from github.GithubException import GithubException

if len(sys.argv) != 2:
    print("Usage: get_c_apps.py <github_token>")
    sys.exit(1)

# Excluded C apps
excluded_apps = [""]

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
                c_apps.append({"app-name": app.name,
                               "build-directory": str(manifest.app.build_directory),
                               "devices": manifest.app.devices.json})

# Print the number of apps to build
print("Nb of apps to build: ", len(c_apps))

# save the list of apps to build in a json format:
with open("c_apps.json", "w", encoding="utf-8") as f:
    f.write(json.dumps(c_apps))
