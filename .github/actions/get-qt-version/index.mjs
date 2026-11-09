import { appendFileSync, existsSync, readFileSync } from 'node:fs';
import { EOL } from 'node:os';

function setOutput(name, value) {
  const file = process.env.GITHUB_OUTPUT;
  if (!file) {
    throw Error('GITHUB_OUTPUT is not set, this action must run inside GitHub Actions');
  }
  appendFileSync(file, `${name}=${value}${EOL}`, 'utf8');
}

function fail(message) {
  const escaped = String(message)
    .replace(/%/g, '%25')
    .replace(/\r/g, '%0D')
    .replace(/\n/g, '%0A');
  process.stdout.write(`::error::${escaped}${EOL}`);
  process.exitCode = 1;
}

try {
  // check if sources are checked out
  if (!existsSync('conanfile.py')) {
    throw Error('No conanfile.py found, please make sure that this action is called after the sources have been checked out');
  }
  const data = readFileSync('conanfile.py', 'utf8');
  const match = data.match(/self\.requires\("qt\/([0-9.]+)"\)/m);
  if (!match) {
    throw Error('No Qt version found in conanfile.py');
  }
  console.log(`Found QT version: ${match[1]}`);
  setOutput('qt-version', match[1]);
} catch (error) {
  fail(error.message);
}
