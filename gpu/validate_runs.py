"""Reject incomplete, duplicate or failed benchmark sets before aggregation."""
import math


def validate(data, powers, variants, *, cold=False):
    if not isinstance(data, dict) or data.get('complete') is not True:
        raise ValueError('benchmark set is not complete')
    rows = data.get('runs', [])
    expected = {(p, v, r) for p in powers for v in variants for r in range(4)}
    actual = set()
    references = {}
    for row in rows:
        key = (row.get('power'), row.get('variant'), row.get('rep'))
        if key not in expected or key in actual:
            raise ValueError('unexpected or duplicate repetition: ' + str(key))
        actual.add(key)
        if row.get('warmup') is not (row['rep'] == 0):
            raise ValueError('invalid warmup flag')
        metric = 'wall_seconds' if cold else 'program_seconds'
        value = row.get(metric)
        if not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
            raise ValueError('invalid timing')
        if cold and row.get('exit_code') != 0:
            raise ValueError('failed cold process')
        stats = row.get('stats')
        if not isinstance(stats, dict) or stats.get('fails') != 0:
            raise ValueError('missing stats or nonzero fails')
        previous = references.setdefault(row['power'], stats)
        if stats != previous:
            raise ValueError('statistics differ within benchmark interval')
    if actual != expected:
        raise ValueError('missing repetitions: ' + str(expected - actual))
    if not cold:
        expected_launches = {(p, v) for p in powers for v in variants}
        found = set()
        for launch in data.get('launches', []):
            key = (launch.get('power'), launch.get('variant'))
            if key not in expected_launches or key in found or launch.get('exit_code') != 0:
                raise ValueError('missing, duplicate or failed launch')
            found.add(key)
        if found != expected_launches:
            raise ValueError('missing launches')
    return rows
