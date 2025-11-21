import colorlog

ch = colorlog.StreamHandler()
ch.setFormatter(colorlog.ColoredFormatter(
	"%(log_color)s%(levelname)-8s%(name)-28s%(reset)s %(blue)s%(message)s",
	datefmt=None,
	reset=True,
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
    log.addHandler(ch)
    return log