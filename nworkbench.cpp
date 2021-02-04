#include "nworkbench.h"

#ifdef TARGET_LPC1768

#endif

#ifdef TARGET_LPC11U35_501

#endif



Ticker PropagateTicker;

nBlockNode * __firstNode = 0;
nBlockNode * __last_node = 0;
nBlockConnection * __first_connection = 0;
nBlockConnection * __last_connection = 0;
uint32_t __propagating = 0;

uint32_t __tickerElapsed = 0;

nBlocks_KernelData __kernel_data = {
    .period = 0.001,
    .tickSource = KERNEL_TICK_TIMER,
    .sourcePin = NC
};

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
void nBlockNode::setKernelData(nBlocks_KernelData kernel_data) { return; }
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
void propagateTick(void) {
    // tickerElapsed is not a boolean flag, it is a counter instead
    // This means two unserviced ticks will force ProgressNodes()
    // to run twice
    // This is a soft realtime system
    __tickerElapsed++;
}

void KernelPeriod(float new_period) {
    // Prevents too low a period
    if (new_period < 0.0001) return;
    
    __kernel_data.period = new_period;
}

void KernelTickSource(nBlocks_KernelSources source_flag, PinName source_pin) {
    __kernel_data.tickSource = source_flag;
    __kernel_data.sourcePin = source_pin;
}

void SetupWorkbench(void) {
    // Broadcast kernel data to all nodes

    // Get first node
    nBlockNode * enode;
    enode = __firstNode;
    
    // Traverse list of nodes
    while (enode != 0) {
        // Broadcast node under cursor
        enode->setKernelData(__kernel_data);
        // Move cursor to next node
        enode = (nBlockNode *)(enode->getNext());
    }

    // Start scheduler
    __tickerElapsed = 0;
    switch (__kernel_data.tickSource) {
        // Initialize the selected tick source
        
        case KERNEL_TICK_TIMER:
            PropagateTicker.attach(&propagateTick, __kernel_data.period);
            break;
            
        case KERNEL_TICK_EXT:
            
            break;
    }
    
}

uint32_t ProgressNodes(void) {
    nBlockConnection * econn;
    nBlockNode * enode;
    
    // Counter to store number of iterations actually processed
    uint32_t num_iterations = 0;
    
    // If __tickerElapsed == 0 this call will return immediately
    while (__tickerElapsed > 0) {
        // Ignore this call if we are in the middle of a frame already
        if (__propagating == 0) {
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

