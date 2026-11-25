# data

Sample covering arrays used as fixtures for exercising the CLI and verifier
against real-world files.

## ipogf

Thirty covering arrays downloaded from NIST's public IPOG-F results
repository (https://math.nist.gov/coveringarrays/ipof/ipof-results.html),
ranging from small (k=3, N=4) to large (k=295, N=277) across several
strengths (t=2..6) and alphabet sizes (v=2..4).

The only change from NIST's own files is the first line: it was rewritten
from NIST's own row-count-only header to this project's `N k <level-spec> t`
parameter line (see `docs/design/ca-file-format.md`), so these files load
directly via `read_covering_array_file`. The data rows themselves, including
NIST's `-` wildcard cells, are untouched.
