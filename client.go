package main

import (
	"bufio"
	"encoding/binary"
	"fmt"
	"math"
	"net"
	"os"
	"strconv"
	"strings"
	"time"
)

func CheckError(err error) {
	if err != nil {
		fmt.Println("Error: ", err)
	}
}

func queryProcess(tcpIPAddress string, tcpPort int) {
	var queryUserInput string

	for !strings.EqualFold(queryUserInput, "quit") {
		queryCommand := make([]byte, 4)
		queryAccountNumber := make([]byte, 4)
		queryUpdateAmount := make([]byte, 4)
		var messageArray [3][]byte
		var numberOfMessages int

		reader := bufio.NewReader(os.Stdin)
		fmt.Print("> ")
		command, _ := reader.ReadString('\n')
		inputArray := strings.SplitAfter(command, " ")

		queryUserInput = strings.TrimSpace(inputArray[0])

		// Client side input validation
		if !strings.EqualFold(queryUserInput, "quit") && ((len(inputArray) < 2 || len(inputArray) > 3) || (!strings.EqualFold(queryUserInput, "query") && !strings.EqualFold(queryUserInput, "update")) || ((strings.EqualFold(queryUserInput, "query") && len(inputArray) != 2) || (strings.EqualFold(queryUserInput, "update") && len(inputArray) != 3))) {
			fmt.Println("Invalid input parameters - please try again")
			continue
		}

		if strings.EqualFold(queryUserInput, "quit") {
			continue
		}

		if strings.EqualFold(queryUserInput, "query") {
			binary.LittleEndian.PutUint32(queryCommand, 1001)
			queryAccountNumberInput, err := strconv.ParseUint(strings.TrimSuffix(inputArray[1], "\n"), 10, 32)
			CheckError(err)
			binary.LittleEndian.PutUint32(queryAccountNumber, uint32(queryAccountNumberInput))
			numberOfMessages = 2
			messageArray[0] = queryCommand
			messageArray[1] = queryAccountNumber
		}

		if strings.EqualFold(queryUserInput, "update") {
			binary.LittleEndian.PutUint32(queryCommand, 1002)
			queryAccountNumberInput, err := strconv.ParseUint(strings.TrimSpace(inputArray[1]), 10, 32)
			binary.LittleEndian.PutUint32(queryAccountNumber, uint32(queryAccountNumberInput))
			var queryUpdateAmountInput float64
			if inputArray[2] != "" {
				queryUpdateAmountInput, err = strconv.ParseFloat(strings.TrimSuffix(inputArray[2], "\n"), 32)
				CheckError(err)
				binary.LittleEndian.PutUint32(queryUpdateAmount, math.Float32bits(float32(queryUpdateAmountInput)))
			}
			numberOfMessages = 3
			messageArray[0] = queryCommand
			messageArray[1] = queryAccountNumber
			messageArray[2] = queryUpdateAmount
		}

		//Connect TCP
		connectionAddress := tcpIPAddress + ":" + strconv.Itoa(tcpPort)
		conn, err := net.Dial("tcp", connectionAddress)
		if err != nil {
			fmt.Println("Connection failed to server at", connectionAddress)
			return
		}

		defer conn.Close()
		//Write query type
		conn.Write(messageArray[0])
		//Read
		queryCommandResponseMessage, _ := bufio.NewReader(conn).ReadString('\n')

		for strings.EqualFold(queryCommandResponseMessage, "COMMAND_RECEIVED\n") {
			//Write account number
			conn.Write(messageArray[1])
			//Read
			accountNumberResponseMessage, _ := bufio.NewReader(conn).ReadString('\n')
			if numberOfMessages == 2 {
				fmt.Print(accountNumberResponseMessage)
			} else {
				for strings.EqualFold(accountNumberResponseMessage, "ACCT_NUM_RECEIVED\n") {
					//Write amount
					conn.Write(messageArray[2])
					//Read
					AmountResponseMessage, _ := bufio.NewReader(conn).ReadString('\n')
					if numberOfMessages == 3 {
						fmt.Print(AmountResponseMessage)
					}
					break
				}
			}
			//reset for next query
			numberOfMessages = 0
			break
		}

	}
	os.Exit(0)
}

func main() {
	// Listen for response from servicemap.go
	serviceListener, err := net.ListenPacket("udp", ":0")
	CheckError(err)
	defer serviceListener.Close()

	//Broadcast Address
	ServerAddr, err := net.ResolveUDPAddr("udp", "255.255.255.255:33104")
	CheckError(err)

	// If client.go does not get a response from service name it will try 2 more times with 30 seconds in between attempts, then exit
	timeoutDuration := 30 * time.Second
	msg := "GET BANK620"
	writeBuf := []byte(msg)
	readBuf := make([]byte, 1024)
	var tcpIPAddress string
	var tcpPort int = 0
	var attemptCounter = 0
	for tcpIPAddress == "" && tcpPort == 0 {
		if attemptCounter > 2 {
			fmt.Println("Service Map not found after", attemptCounter, "attempts. Exiting program.")
			os.Exit(0)
		}
		attemptCounter++
		_, err := serviceListener.WriteTo(writeBuf, ServerAddr)
		CheckError(err)

		serviceListener.SetReadDeadline(time.Now().Add(timeoutDuration))
		n, _, err := serviceListener.ReadFrom(readBuf)
		CheckError(err)

		if strings.EqualFold(string(readBuf[0:n]), "NOT OK") {
			fmt.Println("Database server address not found. Exiting program.")
			os.Exit(0)
		} else {
			//Parsing response into IP address and Port
			msgArray := strings.Split(string(readBuf[0:n]), ",")
			if len(msgArray) == 6 {
				var tcpIPAddress string
				for i := 0; i < 4; i++ {
					tcpIPAddress += msgArray[i]
					if i < 3 {
						tcpIPAddress += "."
					}
				}
				portPartA, err := strconv.Atoi(msgArray[4])
				portPartB, err := strconv.Atoi(strings.TrimSuffix(msgArray[5], "\x00"))
				CheckError(err)
				tcpPort := portPartA*256 + portPartB
				fmt.Println("Service provided by", tcpIPAddress, "at port", tcpPort)

				queryProcess(tcpIPAddress, tcpPort)
			}
		}
	}

}
