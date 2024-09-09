import subprocess

revision = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).decode("utf-8").strip()
status = "clean" if not subprocess.check_output(["git", "status", "--porcelain"]).decode("utf-8").strip() else "dirty"
print(f"'-DGIT_COMMIT=\"{revision}\"' '-DGIT_WORKTREE_STATUS=\"{status}\"'")
