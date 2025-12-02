import { existsSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { execSync } from 'node:child_process'

if (!existsSync(join(import.meta.dirname, 'node_modules'))) {
  execSync('npm install --no-audit --no-package-lock', {
      cwd: import.meta.dirname,
      stdio:[0,1,2]
  });
}

import("@actions/core").then(core => {
  try {
    // check if sources are checked out
    if (!existsSync('conanfile.py')) {
      throw Error("No conanfile.py found, please make sure that this action is called after the sources have been checked out")
    }
    const data = readFileSync('conanfile.py', 'utf8');
    const match = data.match(/self.requires\("qt\/([0-9\.]+)"\)/m);
    if (!match) {
      throw Error("No Qt version found on conanfile.py")
    }
    core.info(`Found QT version: ${match[1]}`);
    core.setOutput("qt-version", match[1]);

  } catch (error) {
    core.setFailed(error.message);
  }
})