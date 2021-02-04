/**
 *  \file nworkbench.h
 *  \author Fernando Cosentino
 *  \brief n-Blocks Studio Kernel main header file
 *  
 *  
 */

#ifndef _NWORKBENCH
#define _NWORKBENCH

#include "mbed.h"
#include "fifo.h"

#define pin_A3    P2_0
#define pin_A4    P2_1
#define pin_A5    P0_10
#define pin_A6    P0_11
#define pin_A7    P0_17
#define pin_A8    P0_18
#define pin_A9    P0_15
#define pin_A10   P0_16

#define pin_B1    P0_3
#define pin_B2    P0_2
#define pin_B3    P1_0
#define pin_B4    P1_1
#define pin_B5    P1_4
#define pin_B6    P1_8
#define pin_B7    P1_9
#define pin_B8    P1_10
#define pin_B9    P1_14
#define pin_B10   P1_15
#define pin_B11   P1_16
#define pin_B12   P1_17

#define pin_C4    P3_26
#define pin_C5    P3_25
#define pin_C6    P0_28
#define pin_C7    P0_27
#define pin_C8    P2_11
#define pin_C9    P0_6
#define pin_C10   P2_6
#define pin_C11   P2_2
#define pin_C12   P2_3

#define pin_D1    P0_23
#define pin_D2    P0_24
#define pin_D3    P0_25
#define pin_D4    P0_26
#define pin_D5    P1_30
#define pin_D6    P1_31
#define pin_D9    P0_1
#define pin_D10   P0_0
#define pin_D11   P0_4
#define pin_D12   P0_5

#define pin_E1    P1_19
#define pin_E2    P1_22
#define pin_E3    P1_25
#define pin_E4    P1_26
#define pin_E5    P1_27
#define pin_E6    P2_13
#define pin_E7    P2_12
#define pin_E8    P0_21
#define pin_E9    P0_22
#define pin_E10   P2_8
#define pin_E11   P2_5
#define pin_E12   P2_4

#define pin_F3    P4_29
#define pin_F4    P4_28
#define pin_F5    P0_19
#define pin_F6    P0_20
#define pin_F7    P0_8
#define pin_F8    P0_9
#define pin_F9    P0_7
#define pin_F10   P1_29

/**
 *  \brief Node output types
 */
enum nBlocks_OutputType {
    OUTPUT_TYPE_INT,
    OUTPUT_TYPE_STRING,
    OUTPUT_TYPE_FLOAT,
    OUTPUT_TYPE_ARRAY
};

/**
 *  \brief Kernel tick input options
 */
enum nBlocks_KernelSources {
    KERNEL_TICK_TIMER,
    KERNEL_TICK_EXT
};

/**
 *  \brief Structure to broadcast kernel data to all nodes prior to 
 *  first frame
 */
 typedef struct nBlocks_KernelData {
     float period;
     uint32_t tickSource;
     PinName sourcePin;
 } nBlocks_KernelData;

/**
 *  Structure data for messages received by destination nodes.
 *  The argument for triggerInput() is of this type.
 */
typedef struct nBlocks_Message {
    uint32_t inputNumber;
    uint32_t intValue;
    float floatValue;
    char * stringValue;
    uint32_t pointerValue;
    uint32_t dataLength;
    nBlocks_OutputType dataType;
    
} nBlocks_Message;

/**
 *  Structure data for a value which corresponds to a numeric ID
 *  Can be used to map a value to a specific index in an array
 */
 typedef struct nBlocks_MappedValue {
     uint32_t index;
     uint32_t value;
 } nBlocks_MappedValue;

/**
 *  \brief Configures the n-Blocks Studio kernel and sets the Ticker.
 *  This function must be called in the main() function,  before entering the main loop.
 */
void SetupWorkbench(void);

/**
 *  \brief Performs all node operations which should take place in each frame.
 *  This function should be polled as frequent as possible, ideally once every main loop
 *  iteration (if that is a fast loop). If the main loop takes more than a few microseconds,
 *  it should be called several times during one iteration, evenly distributed.
 *  Must be called at least at 1 kHz. Safe to be called more that required, as it returns
 *  immediately when not needed.
 *  
 *  \return Number if frames processed during this call. Ideally 0 most of times, 1 every 1ms.
 *  If this function returns more than 1, means it is being called less often than it should
 *  as it had to process more than one frame to catch up.
 */
uint32_t ProgressNodes(void);

/**
 *  \brief Modifies the internal tick period for the kernel, relevant
 *  only if the tick source is an internal timer. Nodes can call this
 *  at will to set the kernel tick period. If more than one node calls
 *  this method,  the last call will be effective.
 *  
 *  \param [in] new_period Period in seconds (as float)
 */
void KernelPeriod(float new_period);

/**
 *  \brief Modifies the kernel tick source. If source is KERNEL_TICK_EXT
 *  the source_pin argument is used as physical interrupt pin, otherwise
 *  source_pin is ignored and can be omitted.
 *  
 *  \param [in] source_flag One of nBlocks_KernelSources constant values
 *      indicating tick source
 *  \param [in] source_pin PinName if source_flag is KERNEL_TICK_EXT
 */
void KernelTickSource(nBlocks_KernelSources source_flag, PinName source_pin);

/**
 *  \brief Packs a float value into an unsigned integer. That is, 
 *  reinterpret the raw bits allowing it to be stored in a variable
 *  of type uint32_t.
 *  
 *  \param [in] value Float value to be packed
 *  \return The bits interpreted as uint32_t
 */
uint32_t PackFloat(float value);


/**
 *  nBlocksNode is the base class for all nodes in n-BlocksStudio.
 *  It handles the input and output basic logic to work with connections.
 *  During frame processing, nodes are traversed in a chain where each
 *  node has  a pointer to the next.
 */
class nBlockNode {
public:
    nBlockNode(void);
    
    /**
     *  \brief Sets the pointer to the next node in the traversing chain.
     *  
     *  \param [in] next The next node in the chain to be stepped after this one
     */
    void setNext(nBlockNode * next);
    
    /**
     *  \brief Retireves the address of the next node in the traversing
     *  chain (set via setNext() ).
     *  
     *  \return 32-bit memory address of the next node. Has to be
     *  externally cast as pointer
     */
    uint32_t getNext(void);
    
    /**
     *  \brief Sets the kernel data received from broadcast prior to
     *  first frame.
     *  
     *  \param [in] kernel_data Struct containing data for the kernel
     */
    void setKernelData(nBlocks_KernelData kernel_data);
    
    /**
     *  \brief Returns the number of data packets available to be read
     *  by a connection object (nBlockConnection class). Called externally
     *  by connection objects. Has to be implemented in descending classes.
     *  
     *  \param [in] outputNumber The output number to check for data
     *  \return Number of messages available. Currently of boolean
     *  interpretation, as the connection reads only one message if this
     *  number is not zero.
     */
    virtual uint32_t outputAvailable(uint32_t outputNumber);
    
    /**
     *  \brief Returns the output type for the given output number.
     *  Output type is one of the constants in the nBlocks_OutputType enum.
     *  Called externally by connection objects. Has to be implemented 
     *  in descending classes.
     *  
     *  \param [in] outputNumber The output number to check for data type
     *  \return One of the constants in the nBlocks_OutputType enum
     */
    virtual nBlocks_OutputType readOutputType(uint32_t outputNumber)  { return OUTPUT_TYPE_INT; }
    
    /**
     *  \brief Returns data from one particular output.
     *  Called externally by connection objects. Has to be implemented 
     *  in descending classes.
     *  
     *  \param [in] outputNumber The output number to retrieve data from
     *  \return The data as 32 bit unsigned integer
     */
    virtual uint32_t readOutput(uint32_t outputNumber);
 
    /**
     *  \brief Receives a value from a connection. Called externally
     *  by connection objects. Has to be implemented in descending 
     *  classes defining the actual behaviour. Typical use is to store
     *  the most recent value to be used in the step() method.
     *  
     *  \param [in] message A nBlocks_Message packet containing
     *      information about the data being received.
     */
    virtual void triggerInput(nBlocks_Message message);
    
    /**
     *  \brief Discards any data respective to previous frame and 
     *  prepares data to be available at the next frame. Also performs
     *  any other actions required for the node behaviour.
     *  
     *  Must rely only on internal variables as no communication with
     *  other nodes or parts of the kernel is allowed in this method.
     */
    virtual void step(void);

private:
    // Pointer to next node in the traversing chain
    nBlockNode * _next;
    
};








/**
 *  \brief Template used by nBlockSimpleNode to set buffer size based on
 *  the number of outputs.
 */
template <size_t simpleNode_OutputSize>

/**
 *  \brief Simplified base class for nodes having output buffers. The majority
 *  of nodes done by users will inherit from this class (instead of
 *  nBlockNode) since the output buffer is almost always used.
 *  
 *  \details Synchronizes the output in frames by using two layers of 
 *  output buffers:
 *    - User modifies output[ <number> ] in user code, while this variable is
 *      never seen externally by connections
 *    - Connections access _exposed_output[ <number> ] while this variable is
 *      never seen or modified by users
 *    - Users no longer modify the step() method directly. Instead, users
 *      implement the endFrame() method, with same purpose
 *    - The step() method invokes endFrame(), and then copies the contents
 *      from output[] to _exposed_output[], when the user is no longer
 *      able to modify it and connections are not active
 *  
 *  Implementation code has to be in this file (instead of nworkbench.cpp)
 *  due to use of the template feature.
 */
class nBlockSimpleNode: public nBlockNode {
public:
    /**
     *  \brief Constructor for nBlockSimpleNode, initializes all output 
     *  buffers (output[] and available[] as well as their exposed counterparts)
     */
    nBlockSimpleNode(void) {
        unsigned int i;
        for (i=0; i<simpleNode_OutputSize; i++) {
            // Buffers visible to connections, not users
            _exposed_output[i] = 0;
            _exposed_available[i] = 0;
            // Buffers accessible to users, not connections
            output[i] = 0;
            available[i] = 0;
            // Buffers not modified after instantiation
            outputType[i] = OUTPUT_TYPE_INT; // Output type defaults to integer
        }
    }
    
    /**
     *  \brief Called after kernel data is received and stored in the
     *  kernelData private property. The user (node developer) should
     *  implement this method if there are particular actions to be
     *  taken when the kernel data changes (e.g. tick period), such as
     *  configuring libraries or external devices.
     *  When this method is called, the kernelData property is ready.
     *  
     */
    virtual void onKernelData() { return; }
    
    /**
     *  \brief Stores the received kernel data into the private
     *  kernelData property, and invokes the onKernelData method.
     *  
     *  \param [in] kernel_data Struct containing data for the kernel
     */
    void setKernelData(nBlocks_KernelData kernel_data) {
        kernelData = kernel_data;
        onKernelData();
    }
    
    
    /**
     *  \brief Called to check if there is data available at one particular
     *  output number. Data can be regarded as either a flag (data available
     *  or not available) or a number of messages available. In any case,
     *  only the value set at output (and _exposed_output) will be read
     *  by readOutputs. If the output data type is string, the value
     *  is the length of the string in the buffer.
     *  This method should not be modified except in very specific cases.
     *  
     *  \param [in] outputNumber The output number to check for data
     *  \return Zero if no messages available, non-zero otherwise.
     */
    uint32_t outputAvailable(uint32_t outputNumber) { return _exposed_available[outputNumber]; }
    
    /**
     *  \brief Returns the output type for the given output number.
     *  Output type is one of the constants in the nBlocks_OutputType enum.
     *  This method should not be modified except in very specific cases.
     *  
     *  \param [in] outputNumber The output number to check for data type
     *  \return One of the constants in the nBlocks_OutputType enum
     */
    nBlocks_OutputType readOutputType(uint32_t outputNumber) { return outputType[outputNumber]; }
    
    /**
     *  \brief Returns data stored at the output buffer exposed to
     *  connections (_exposed_output). This method is used when the
     *  data type is integer, as it only returns one value (the most
     *  recent).
     *  This method should not be modified except in very specific cases.
     *  
     *  \param [in] outputNumber The output number to retrieve data from
     *  \return The data as 32 bit unsigned integer
     */
    uint32_t readOutput(uint32_t outputNumber) { return _exposed_output[outputNumber]; }
    
    /**
     *  \brief The user (node developer) *must* implement the endFrame()
     *  method if any actions should be performed by the node at the end
     *  of each frame. This replaces the step() method if the node is of
     *  nBlockSimpleNode class (the actual step() method should never be
     *  modified in this case).
     *  
     *  If the user does not implement this method, the node will not
     *  perform any actions in the end of the frame ("stepping" this node
     *  will have no effect). The majority of use cases will make use of
     *  this method.
     */
    virtual void endFrame(void) { return; }

    /**
     *  \brief Invokes the user code at the endFrame() method, and
     *  moves the content of the user buffers (output[] and available[])
     *  to the buffers exposed to connections (_exposed_output[] and
     *  _exposed_available[]).
     *  This method is called automatically by the kernel, and should not
     *  be called manually in any circumstance. Should not be modified
     *  in nodes inherited from nBlockSimpleNode - the node developer
     *  should modify endFrame() instead.
     */
    void step(void) {
        unsigned int i;
        endFrame();
        for (i=0; i<simpleNode_OutputSize; i++) {
            _exposed_output[i] = output[i];
            _exposed_available[i] = available[i];
            available[i] = 0;
        }
        return;
    }
        
    /**
     *  \brief Buffer holding output data, to be modified by user.
     */
    uint32_t output[simpleNode_OutputSize];
    
    /**
     *  \brief Buffer holding output types. Should be written 
     *  in constructor only. Defaults to OUTPUT_TYPE_INT
     */
    nBlocks_OutputType outputType[simpleNode_OutputSize];
    
    /**
    *  \brief Buffer holding data availability, to be modified by user.
     */
    uint32_t available[simpleNode_OutputSize];
private:

    /**
     *  \brief Struct holding data received in kernel data broadcasting
     */
    nBlocks_KernelData kernelData;

    /**
     *  \brief Buffer holding output data, exposed to connections and
     *  hidden from user. Do not modify the contents of this buffer in 
     *  node code.
     */
    uint32_t _exposed_output[simpleNode_OutputSize];
    
    /**
     *  \brief Buffer holding data availability, exposed to connections and
     *  hidden from user. Do not modify the contents of this buffer in 
     *  node code.
     */
    uint32_t _exposed_available[simpleNode_OutputSize];
};








/**
 *  \brief Class representing a connection between one output from a source 
 *  node and one input in a destination node.
 *  This class should not be derived or modified. It is just instantiated
 *  as connections as is.
 */
class nBlockConnection {
public:
    /**
     *  \brief Constructor takes 4 parameters (nodes and numbers for 
     *  inputs and outputs). Data type is automatically detected from
     *  source node. Also detects if the instance being constructed is
     *  the first instance in the kernel, and automatically sets itself
     *  as entry point for connection traversing.
     *  
     *  \param [in] srcBlock Source node
     *  \param [in] outputNumber The output number to read data from
     *  \param [in] dstBlock Destination node
     *  \param [in] inputNumber The input number to write data to
     */
    nBlockConnection(nBlockNode * srcBlock, uint32_t outputNumber, nBlockNode * dstBlock, uint32_t inputNumber);
    
    /**
     *  \brief Moves data across the connection, that is, reads data
     *  from the source node using the output number, and writes it into
     *  the destination node using the input number.
     */
    void propagate(void);
    
    /**
     *  \brief Sets the pointer to the next connection object in the 
     *  traversing chain.
     *  
     *  \param [in] next The next connection in the chain to be propagated
     *  after this one
     */
    void setNext(nBlockConnection * next);

    /**
     *  \brief Retireves the address of the next connection in the traversing
     *  chain (set via setNext() ).
     *  
     *  \return 32-bit memory address of the next connection. Has to be
     *  externally cast as pointer
     */
    uint32_t getNext(void);
private:
    /** Pointer holding the source node given in the constructor */
    nBlockNode * _srcBlock;
    /** Holds the output number given in the constructor */
    uint32_t _outputNumber;
    /** Pointer holding the destination node given in the constructor */
    nBlockNode * _dstBlock;
    /** Holds the input number given in the constructor */
    uint32_t _inputNumber;
    /** Pointer holding the next connection object in the traversing chain */
    nBlockConnection * _next;
};


#endif
