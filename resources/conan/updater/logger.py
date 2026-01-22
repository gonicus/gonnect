import colorlog
import os
import sys

format_string = "%(log_color)s%(levelname)-8s%(name)-28s%(reset)s %(blue)s%(message)s"
reset = True
if "GITHUB_ACTION" in os.environ:
    format_string = "%(levelname)-8s%(name)-28s %(message)s"
    reset = False

ch = colorlog.StreamHandler(stream=sys.stdout)
ch.setFormatter(colorlog.ColoredFormatter(
	format_string,
	datefmt=None,
	reset=reset,
	log_colors={
		'DEBUG':    'cyan',
		'INFO':     'green',
		'WARNING':  'yellow',
		'ERROR':    'red',
		'CRITICAL': 'red,bg_white',
	},
	secondary_log_colors={},
	style='%'
))
logger = colorlog.getLogger('updater')
logger.setLevel(colorlog.INFO)
logger.addHandler(ch)

def create_logger(name, level=None):
    log = colorlog.getLogger(name)
    if level is not None:
        log.setLevel(level)
    else:
        log.setLevel(logger.level)
    log.addHandler(ch)
    return log