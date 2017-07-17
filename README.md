# Peer To Peer File Transfer

Transfer file to any peer in same Network

## Installation

make
	
	>exe directory will be created

## Uninstallation

make clean

	
## Usage

./exe/peer

-> For Local Host
It will ask you if you want run run peers on localhost

Enter Peer Number[0-255]: 0
Do You want to test With Local Host [y/n] : Y  (Y to test In Local Host)
Enter Port Number For [This PEER]: 7890		   (Enter This Peer Port Number, For Local Host Only)

(Option Menu Will Popup, Option 1 not Supported Now )
		-----------------------------------------
		| 1. Request for download File from Peer |
		| 2. Transfer File                       |
		| 3. Connect/Discover Peer in Network    |
		-----------------------------------------
		| Enter: 

## STEP To Connect With Peer

First Connect with Peer : Enter 3, It will ask you to enter Peer IP Address (If you selected Local host it will ask you for the Peer Port Number)
Enter Peer Port Number : 3890   (For Local Host, For Other It will ask you to enter IP address with (.)dot)

After this You will be Redirected to Main Menu
	-----------------------------------------
	| 1. Request for download File from Peer |
	| 2. Transfer File                       |
	| 3. Connect/Discover Peer in Network    |
	-----------------------------------------
	| Enter: 

## STEP To Transfer File
Now Enter 2 for Transfer file, It will ask you to enter file name you want to transfer
Enter File Name (25 letters): test.txt 
Number of Byte(s) 33

	-----------------------------------------
	| 1. Request for download File from Peer |
	| 2. Transfer File                       |
	| 3. Connect/Discover Peer in Network    |
	-----------------------------------------
	| Enter: 2

Enter File Name (25 letters): show_discovery.png
Number of Byte(s) 174093

Block(s) Transferred:341/341


Other Peer :- 
New file request from Peer [show_discovery.png]
Block(s) Remaining: 00000001/341
CheckSum Calculated :56e024503bdd004d7c81ead4cf711f4e
CheckSum Received   :56e024503bdd004d7c81ead4cf711f4e
Block(s) Remaining: 00000000/341
File Download Success [show_discovery.png]




##  To run on Network

It will ask you if you want run run peers on localhost

Enter Peer Number[0-255]: 0
Do You want to test With Local Host [y/n] : n 

Peer File Transfer Starting on default port (7890)
Creating On Interface

Adding multicast group 			[OK]
Setting the local interface 	[OK]
Disabling the loopback 			[OK]
Starting Neighbour Discovery 	[OK]
Discover Packet len = [28]			<------- When Application starts, it will sends Discovery packet 
Sending Discover message 		[OK]         and other node will update thier IP table and response back to this peer

DISCOVERY REQUEST From Peer[1]			<----- Discovery Packet from Other Peer
Neighbour Added IP:[192.168.0.109] To IP Table <---- Update IP Table
Sending Discover message 		[OK]		   <---- Send Discovery Packet response 

	-----------------------------------------
	| 1. Request for download File from Peer |
	| 2. Transfer File                       |
	| 3. Connect/Discover Peer in Network    |
	-----------------------------------------
	| Enter: 3 (Show Discovered IP)

	| 1. Enter IP Manually       |
	| 2. Show Discovered IP list |
	     Enter: 2

	------------------------------
	|NODE No |    IP ADDRESS     | 
	|   1    |    192.168.0.109  | <---\__IP Table__| It will update whenever
	|   2    |    192.168.0.111  | <---/            | you peer will connect in network
	------------------------------
Set Peer Info (Enter Node ID):1  <--------------< Enter Node No You Want to Send File 

	-----------------------------------------
	| 1. Request for download File from Peer |
	| 2. Transfer File                       |
	| 3. Connect/Discover Peer in Network    |
	-----------------------------------------
	| Enter: 2						<--------------< Transfer File

	-----------------------------------------
	| 1. Request for download File from Peer |
	| 2. Transfer File                       |
	| 3. Connect/Discover Peer in Network    |
	-----------------------------------------
	| Enter: 2

Enter File Name (25 letters): show_discovery.png
Number of Byte(s) 174093

Block(s) Transferred:341/341

Other Peer :- 
New file request from Peer [show_discovery.png]
Block(s) Remaining: 00000001/341
CheckSum Calculated :56e024503bdd004d7c81ead4cf711f4e
CheckSum Received   :56e024503bdd004d7c81ead4cf711f4e
Block(s) Remaining: 00000000/341
File Download Success [show_discovery.png]


## NOTE
For Local Host, run other peer(s) with different path and different port number as File name will be same.

THIS APPLICATION IS FULLY TESTED ON LOCALHOST, ON NETWORK WITH OTHER PEERS ARE NOT FULLY TESTED (AS I ONLY HAVE 2 SYSTEM 1 PC and 2 Raspberry PI)

SCREENSHOT IN screenshot folder



Ref:-
https://tools.ietf.org/html/rfc1350 
http://www.tldp.org/HOWTO/Multicast-HOWTO-6.html
https://austinmarton.wordpress.com/2011/09/14/sending-raw-ethernet-packets-from-a-specific-interface-in-c-on-linux/
