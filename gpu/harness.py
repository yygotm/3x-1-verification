"""Bounded pipe reads and unconditional child cleanup for benchmark drivers."""
import queue
import subprocess
import threading
import time


class Responses:
    def __init__(self, process, logfile):
        self.process = process
        self.logfile = logfile
        self.lines = queue.Queue()
        self.reader = threading.Thread(target=self._pump, daemon=True)
        self.reader.start()

    def _pump(self):
        try:
            for line in self.process.stdout:
                self.lines.put(line)
        finally:
            self.lines.put(None)

    def read(self, prefix, timeout):
        deadline = time.monotonic() + timeout
        lines = []
        while True:
            try:
                line = self.lines.get(timeout=max(0, deadline - time.monotonic()))
            except queue.Empty:
                raise TimeoutError('response timeout: ' + prefix) from None
            if line is None:
                raise RuntimeError('EOF before ' + prefix)
            self.logfile.write(line)
            self.logfile.flush()
            lines.append(line)
            if line.startswith(prefix):
                return ''.join(lines)

    def close(self):
        self.reader.join(timeout=5)
        if self.reader.is_alive():
            raise RuntimeError('stdout reader did not stop after process cleanup')
        # Preserve any partial output, including on timeout or EOF.
        while not self.lines.empty():
            line = self.lines.get_nowait()
            if line is not None:
                self.logfile.write(line)
        self.logfile.flush()
        self.process.stdout.close()


def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)
    if process.stdin and not process.stdin.closed:
        process.stdin.close()
