# data

Sample covering arrays used as fixtures for exercising the CLI and verifier
against real-world files.

## ipogf

Fifteen small/medium covering arrays downloaded from NIST's public IPOG-F
results repository (https://math.nist.gov/coveringarrays/ipof/ipof-results.html).

The only change from NIST's own files is the first line: it was rewritten
from NIST's own row-count-only header to this project's `N k <level-spec> t`
parameter line (see `docs/design/ca-file-format.md`), so these files load
directly via `read_covering_array_file`. The data rows themselves, including
NIST's `-` wildcard cells, are untouched.
