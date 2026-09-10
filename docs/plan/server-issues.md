# server issues

## Tasks

Create a doc with options + pros/cons and recommendations + why for all of these issues.

### SI1 add redoc page

- maximally annotated to maximize detailed type and api bdehavior in redoc page's definiotions
- `/redoc`

### SI2 add coverage report HTML to the pages site when deploying pages

add the generated html coverage report to the GH pages site

- do we generate results and coverage reports per environment branch (i.e. development, staging, main, release)?
- post per-branch for development, staging, main, release (not main)

### SI3 do we need the main branch?

it seems like development, staging, main, release are only ones we need. Actually, ideally we woulb eusing production instead of main. Dont change unless risk is totally nontrivial.

### SI4 grpc tls

### SI5a Need unified overall authentication mechanism that is more secure than Basic we hav for REST and nothing we have for grpc

### SI5b Need Authorization scheme based on loggedin user and reosurce by resource (grpc and REST). Both communication channel type AND specific data resources (e.g. only designs you have uploaded or are shared to all users). Needregular interactive client scheme AND device flow in addition to for machine -> server auth

### SI6 Server-side optimizations- Any optimizations options? (client design fetching, loading, display, and visibility toggle has been heavily optimized (> 10x faster fech-?load->display and almost instantaneous componenst  and layer visibility toggling) ?

### SI7 RequestLoadDesign(Async) strart a filarchive/Design load (parse) and return immediately. next GetDesignAsync request for that same design can skip loading and return response data immediately

### SI8 server communication channel- non-language-specific: SSE? websockets? optional connection and use- app still functions, falls back even if connection is not attempted or attempted but cant complete successfully

### SI19 remaining grpc optimzation items from `docs/plan/opt/phase2-grpc-optimizations.md`

implement remaining issues from the previous grpc optimzaiton round. 

Discussed here: `docs/plan/opt/phase2-grpc-optimizations.md`
