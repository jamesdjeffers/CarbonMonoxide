"""
Async callbacks with a queue and external consumer
--------------------------------------------------
An example showing how async notification callbacks can be used to
send data received through notifications to some external consumer of
that data.
Created on 2021-02-25 by hbldh <henrik.blidh@nedomkull.com>
"""
import sys
import time
import platform
import asyncio
import logging

from bleak import BleakClient

logger = logging.getLogger(__name__)

ADDRESS = (
    "75:6F:61:48:18:5F"
    if platform.system() != "Darwin"
    else "19B10001-E8F4-537E-4F6C-D104768A1214"
)
CHARACTERISTIC_UUID = "19B10001-E8F4-537E-4F6C-D104768A1214"


async def run_ble_client(address: str, char_uuid: str, queue: asyncio.Queue):
    async def callback_handler(_, data):
        await queue.put((time.time(), data))

    async with BleakClient(address) as client:
        logger.info(f"Connected: {client.is_connected}")
        await client.start_notify(char_uuid, callback_handler)
        await asyncio.sleep(10.0)
        await client.stop_notify(char_uuid)
        # Send an "exit command to the consumer"
        await queue.put((time.time(), None))


async def run_queue_consumer(queue: asyncio.Queue):
    while True:
        # Use await asyncio.wait_for(queue.get(), timeout=1.0) if you want a timeout for getting data.
        epoch, data = await queue.get()
        if data is None:
            logger.info(
                "Got message from client about disconnection. Exiting consumer loop..."
            )
            break
        else:
            logger.info(f"Received callback data via async queue at {epoch}: {data}")


async def main(address: str, char_uuid: str):
    queue = asyncio.Queue()
    client_task = run_ble_client(address, char_uuid, queue)
    consumer_task = run_queue_consumer(queue)
    await asyncio.gather(client_task, consumer_task)
    logger.info("Main method done.")


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    try:
        asyncio.run(
            main(
                sys.argv[1] if len(sys.argv) > 1 else ADDRESS,
                sys.argv[2] if len(sys.argv) > 2 else CHARACTERISTIC_UUID,
            )
        )
    except:
        logger.info("ERROR")
    logger.info("DONE")
