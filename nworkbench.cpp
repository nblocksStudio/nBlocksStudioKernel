#include "nworkbench.h"

#ifdef TARGET_LPC1768

#endif

#ifdef TARGET_LPC11U35_501

#endif


#ifdef ARDUINO
	uint32_t __currentTick;
#else
	Ticker PropagateTicker;
#endif

nBlockNode * __firstNode = 0;
nBlockNode * __last_node = 0;
nBlockConnection * __first_connection = 0;
nBlockConnection * __last_connection = 0;
uint32_t __propagating = 0;

uint32_t __tickerElapsed = 0;


uint32_t PackFloat(float value) {
	uint32_t packed = 0;
	// Copy memory contents of value into packed
	memcpy(&packed, &value, sizeof(packed));
	// Return packed
	return packed;
}



// NBLOCK NODE BASIC CLASS
nBlockNode::nBlockNode(void) {
    // placeholder
    if (__firstNode == 0) __firstNode = this;
    if (__last_node != 0) __last_node->setNext(this);
    __last_node = this;
}
void nBlockNode::setNext(nBlockNode * next) { this->_next = next; }
uint32_t nBlockNode::getNext(void) { return (uint32_t)(this->_next); }
uint32_t nBlockNode::outputAvailable(uint32_t outputNumber) { return 0; }
uint32_t nBlockNode::readOutput(uint32_t outputNumber) { return 0; }
void nBlockNode::triggerInput(nBlocks_Message message) { return; }
void nBlockNode::step(void) { return; }




// NBLOCKCONNECTION
nBlockConnection::nBlockConnection(nBlockNode * srcBlock, uint32_t outputNumber, nBlockNode * dstBlock, uint32_t inputNumber) {
    this->_srcBlock = srcBlock;
    this->_outputNumber = outputNumber;
    this->_dstBlock = dstBlock;
    this->_inputNumber = inputNumber;
    this->_next = 0;

    if (__first_connection == 0) __first_connection = this;
    if (__last_connection != 0) __last_connection->setNext(this);
    __last_connection = this;
}

void nBlockConnection::propagate(void) {
    uint32_t data_available;
    nBlocks_OutputType data_type;
	nBlocks_Message message;
    
    // Check if the connected output has data available
    data_available = this->_srcBlock->outputAvailable(this->_outputNumber);
    if (data_available > 0) {
        // Data is available. If the output type is string or array, 
		// this is number of chars/values to be read. 
		// Otherwise, this is a boolean flag
		        
        // Retrieve data type
        data_type = this->_srcBlock->readOutputType(this->_outputNumber);

		// Populate message fields
		message.inputNumber = this->_inputNumber;
		message.dataType = data_type;
		message.dataLength = data_available;
		// Reset all values
		message.intValue = 0;
		message.floatValue = 0.0;
		char * empty_string = (char *)(""); // 1-char string with null
		message.stringValue = empty_string;
		
		// Assign the correct value based on type
		
		switch (data_type) {
			case OUTPUT_TYPE_INT:
				// INT type is direct read
				message.intValue = this->_srcBlock->readOutput(this->_outputNumber);
				break;

			case OUTPUT_TYPE_STRING:
				// STRINGs are passed as uint memory addresses (char *)
				message.stringValue = (char *)(this->_srcBlock->readOutput(this->_outputNumber));
				break;

			case OUTPUT_TYPE_ARRAY:
				// ARRAYs are passed as uint memory addresses
				message.pointerValue = (this->_srcBlock->readOutput(this->_outputNumber));
				break;
				
			case OUTPUT_TYPE_FLOAT:
				uint32_t packed_value = (this->_srcBlock->readOutput(this->_outputNumber));
				// Copy memory contents of the output value into the
				// message field, reinterpreting the bits
				memcpy(&(message.floatValue), &(packed_value), sizeof(packed_value));
				break;
		}

		// Finally trigger the message in the receiving node
		this->_dstBlock->triggerInput(message);
    }
}
void nBlockConnection::setNext(nBlockConnection * next) {
    this->_next = next;
}
uint32_t nBlockConnection::getNext(void) {
    return (uint32_t)(this->_next);
}


///////////////////
// Core periodic functions

// tickerElapsed is not a boolean flag, it is a counter instead
// This means two unserviced ticks will force ProgressNodes()
// to run twice
// This is a soft realtime system

#ifdef ARDUINO
	// Arduino toolchain
	void SetupWorkbench(void) {
		init();
		#if defined(USBCON)
		USBDevice.attach();
		#endif		
		__tickerElapsed = 0;
		__currentTick = millis();
	}
	
	// In Arduino toolchain, checkTickerElapsed does both tasks:
	// advancing the frame time and returning the value
	uint32_t checkTickerElapsed() {
		// millis() will overflow every 50 days, so we must handle
		// the case when millis() is lower than __currentTick
		uint32_t millis_value = millis();
		uint32_t delta_time_ms;
		if (millis_value < __currentTick) {
			// Overflow case: we need the millis_value as well as the
			// remaining value from __currentTick before the overflow
			
			// Get the last valid value before the overflow. 
			// In a fast enough system, __currentTick will be holding 
			// 0xFFFFFFFF which means this will result zero
			uint32_t remaining = 0xFFFFFFFF - __currentTick; 
			// Add the tick which caused the overflow. This is in
			// a separate instruction to avoid actually overflowing
			// in the operation above
			remaining += 1; 
			// Now remaining is the amount of time elapsed up to the
			// overflow itself. In a fast enough system millis() will
			// be zero as we just hit overflow. If it is not, add
			// the extra time elapsed
			delta_time_ms = remaining + millis_value;
			
		}
		else {
			// Normal case: simple delta
			delta_time_ms = millis() - __currentTick;
		}
		// In current version frequency is fixed at 1kHz so we just
		// add the number of milliseconds elapsed
		// In the vast majority of calls delta_time_ms will be zero
		__tickerElapsed += delta_time_ms;
		
		// Return the updated value
		return __tickerElapsed;
	}
	
	
#else
	// Default toolchain is mbed

	void SetupWorkbench(void) {
		__tickerElapsed = 0;
		PropagateTicker.attach(&propagateTick, 0.001);
	}

	// In mbed toolchain, advancing the frame time is done by
	// propagateTick while checkTickerElapsed only returns the value
	// This is due to propagateTick being a callback from a Ticker
	void propagateTick(void) {
		__tickerElapsed++;
	}
	uint32_t checkTickerElapsed() {
		return __tickerElapsed;
	}
#endif


uint32_t ProgressNodes(void) {
    nBlockConnection * econn;
    nBlockNode * enode;
    
	// Counter to store number of iterations actually processed
    uint32_t num_iterations = 0;
    
	// checkTickerElapsed() returns __tickerElapsed
	// (in Arduino toolchain it also updates if before returning)
    // If __tickerElapsed == 0 this call will return immediately
    while (checkTickerElapsed() > 0) {
		// Ignore this call if we are in the middle of a frame already
        if ((__propagating == 0) && (__first_connection != 0)) {
            __propagating = 1; // Flag: we are in the middle of a frame

			// Remove one frame tick from the down counter
			__tickerElapsed--;
			// Add one iteration to the up counter (return value)
			num_iterations++;

            // --------
			// Propagate connections
			
            // Get cursor to first connection (connection stage entry point)
			econn = __first_connection;
			// Traverse list of connections
            while (econn != 0) {
				// Propagate connection under cursor
                econn->propagate();
				// Move cursor to next connection
                econn = (nBlockConnection *)(econn->getNext());
            }
            
			// --------
            // Step blocks' state machines and fifos
            
			// Get cursor to first node (step stage entry point)
			enode = __firstNode;
			// Traverse list of nodes
            while (enode != 0) {
				// Step node under cursor
                enode->step();
				// Move cursor to next node
                enode = (nBlockNode *)(enode->getNext());
            }
            __propagating = 0; // Flag: no longer inside a frame
        }
    }
	// Return the number of frames that were actually processed
    return num_iterations;
}

