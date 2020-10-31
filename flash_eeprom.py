import os, math
import serial, sys
import asyncio, serial_asyncio

# Print iterations progress
# Borrowed from https://stackoverflow.com/questions/3173320/text-progress-bar-in-the-console
def printProgressBar(iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

async def send(writer, reader, rom_file):
	await asyncio.sleep(1)

	command = str.encode('2\n')
	writer.write(command)

	await asyncio.sleep(0.5)

	progressLength = math.ceil(os.path.getsize(rom_file) / 64)

	with open(rom_file, 'rb') as f:
		i = 0
		for chunk in iter(lambda: f.read(64), b''):

			writer.write(chunk)
			await writer.drain()

			# wait for ACK flag
			await reader.readuntil(b'\x06') 

			i += 1
			printProgressBar(i, progressLength, prefix = 'Progress:', suffix = 'Complete', length = 50)

async def process(dev, rom_file):
	reader, writer = await serial_asyncio.open_serial_connection(url=dev, baudrate=115200)
	sender = send(writer, reader, rom_file)
	await asyncio.wait([sender])

def main(args):
	dev = args[0]
	rom_file = args[1]

	loop = asyncio.get_event_loop()
	loop.run_until_complete(process(dev, rom_file))
	loop.close()

if __name__ == "__main__":
   main(sys.argv[1:])