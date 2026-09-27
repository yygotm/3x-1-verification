/* Used after donestat_t is declared. Malformed checkpoint lines are ignored.
 * Integer conversion is bounded even for corrupted or adversarial log input. */
static int checkpoint_u64(const char **cursor, uint64_t *value) {
    const char *p = *cursor;
    while (*p == ' ') p++;
    if (*p < '0' || *p > '9') return 0;
    errno = 0; char *end;
    unsigned long long v = strtoull(p, &end, 10);
    if (errno || v > UINT64_MAX || (*end && *end != ' ' && *end != '\n' && *end != '\r')) return 0;
    *cursor = end; *value = (uint64_t)v; return 1;
}
static int checkpoint_field(const char **cursor, const char *name, uint64_t *value) {
    const char *p = *cursor;
    while (*p == ' ') p++;
    size_t len = strlen(name);
    if (strncmp(p, name, len)) return 0;
    p += len;
    if (!checkpoint_u64(&p, value)) return 0;
    *cursor = p; return 1;
}
static int parse_done(const char *line, const char *tag, size_t chunk, size_t nfront, size_t *ci, donestat_t *d) {
    if (!chunk || strncmp(line, "DONE ", 5)) return 0;
    const char *p = line + 5; uint64_t x, y, ignored, ms;
    if (!checkpoint_u64(&p, &x) || !checkpoint_u64(&p, &y)) return 0;
    while (*p == ' ') p++;
    size_t len = strlen(tag);
    if (strncmp(p, tag, len) || p[len] != '|') return 0;
    p += len + 1;
    if (x >= nfront || x % chunk || y != x + (chunk < nfront - x ? chunk : nfront - x)) return 0;
    uint64_t di, dp, leaf, fails, argmax;
    if (!checkpoint_field(&p,"nodes=",&ignored) || !checkpoint_field(&p,"dropI=",&di) ||
        !checkpoint_field(&p,"dropP=",&dp) || !checkpoint_field(&p,"leafn=",&leaf) ||
        !checkpoint_field(&p,"skipn=",&ignored) || !checkpoint_field(&p,"traced=",&ignored) ||
        !checkpoint_field(&p,"stopP=",&ignored) || !checkpoint_field(&p,"fails=",&fails) ||
        !checkpoint_field(&p,"maxsteps=",&ms) || !checkpoint_field(&p,"at n=",&argmax) || ms > LONG_MAX) return 0;
    *ci = (size_t)(x / chunk);
    *d = (donestat_t){di, dp, leaf, fails, (long)ms, argmax};
    return 1;
}
