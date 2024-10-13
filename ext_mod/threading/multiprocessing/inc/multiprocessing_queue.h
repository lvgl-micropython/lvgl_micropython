#include "freertos/message_buffer.h"


UBaseType_t uxQueueMessagesWaiting( QueueHandle_t xQueue );
UBaseType_t uxQueueSpacesAvailable( QueueHandle_t xQueue );
void vQueueDelete( QueueHandle_t xQueue );

BaseType_t xQueueReset( QueueHandle_t xQueue );

 QueueHandle_t xQueueCreateStatic(
                             UBaseType_t uxQueueLength,
                             UBaseType_t uxItemSize,
                             uint8_t *pucQueueStorageBuffer,
                             StaticQueue_t *pxQueueBuffer );


/* The queue is to be created to hold a maximum of 10 uint64\_t
   variables. */
#define QUEUE_LENGTH    10
#define ITEM_SIZE       sizeof( uint64_t )

/* The variable used to hold the queue's data structure. */
static StaticQueue_t xStaticQueue;

/* The array to use as the queue's storage area. This must be at least
   uxQueueLength * uxItemSize bytes. */
uint8_t ucQueueStorageArea[ QUEUE_LENGTH * ITEM_SIZE ];

void vATask( void *pvParameters )
{
QueueHandle_t xQueue;

    /* Create a queue capable of containing 10 uint64_t values. */
    xQueue = xQueueCreateStatic( QUEUE_LENGTH,
                                 ITEM_SIZE,
                                 ucQueueStorageArea,
                                 &xStaticQueue );

    /* pxQueueBuffer was not NULL so xQueue should not be NULL. */
    configASSERT( xQueue );
 }






BaseType_t xQueueReceive(
                          QueueHandle_t xQueue,
                          void *pvBuffer,
                          TickType_t xTicksToWait
);


/* Define a variable of type struct AMMessage. The examples below demonstrate
   how to pass the whole variable through the queue, and as the structure is
   moderately large, also how to pass a reference to the variable through a queue. */
struct AMessage
{
    char ucMessageID;
    char ucData[ 20 ];
} xMessage;

/* Queue used to send and receive complete struct AMessage structures. */
QueueHandle_t xStructQueue = NULL;

/* Queue used to send and receive pointers to struct AMessage structures. */
QueueHandle_t xPointerQueue = NULL;

void vCreateQueues( void )
{
    xMessage.ucMessageID = 0xab;
    memset( &( xMessage.ucData ), 0x12, 20 );

    /* Create the queue used to send complete struct AMessage structures. This can
       also be created after the schedule starts, but care must be task to ensure
       nothing uses the queue until after it has been created. */
    xStructQueue = xQueueCreate(
        /* The number of items the queue can hold. */
        10,
        /* Size of each item is big enough to hold the<br /> whole structure. */
        sizeof( xMessage ) );

    /* Create the queue used to send pointers to struct AMessage structures. */
    xPointerQueue = xQueueCreate(
        /* The number of items the queue can hold. */
        10,
        /* Size of each item is big enough to hold only a pointer. */
        sizeof( &xMessage ) );

    if( ( xStructQueue == NULL ) || ( xPointerQueue == NULL ) )
    {
        /* One or more queues were not created successfully as there was not enough
           heap memory available. Handle the error here. Queues can also be created
           statically. */
    }
}

/* Task that writes to the queues. */
void vATask( void *pvParameters )
{
    struct AMessage *pxPointerToxMessage;

    /* Send the entire structure to the queue created to hold 10 structures. */
    xQueueSend( /* The handle of the queue. */
                xStructQueue,
                /* The address of the xMessage variable. sizeof( struct AMessage )
                    bytes are copied from here into the queue. */
                ( void * ) &xMessage,
                /* Block time of 0 says don't block if the queue is already full.
                   Check the value returned by xQueueSend() to know if the message
                   was sent to the queue successfully. */
                ( TickType_t ) 0 );

    /* Store the address of the xMessage variable in a pointer variable. */
    pxPointerToxMessage = &xMessage;

    /* Send the address of xMessage to the queue created to hold 10 pointers. */
    xQueueSend( /* The handle of the queue. */
                xPointerQueue,
                /* The address of the variable that holds the address of xMessage.
                   sizeof( &xMessage ) bytes are copied from here into the queue. As the
                   variable holds the address of xMessage it is the address of xMessage
                   that is copied into the queue. */
                ( void * ) &pxPointerToxMessage,
                ( TickType_t ) 0 );

    /* ... Rest of task code goes here. */
}

/* Task that reads from the queues. */
void vADifferentTask( void *pvParameters )
{
    struct AMessage xRxedStructure, *pxRxedPointer;

    if( xStructQueue != NULL )
    {
        /* Receive a message from the created queue to hold complex struct AMessage
           structure. Block for 10 ticks if a message is not immediately available.
           The value is read into a struct AMessage variable, so after calling
           xQueueReceive() xRxedStructure will hold a copy of xMessage. */
        if( xQueueReceive( xStructQueue,
                           &( xRxedStructure ),
                           ( TickType_t ) 10 ) == pdPASS )
        {
            /* xRxedStructure now contains a copy of xMessage. */
        }
    }

    if( xPointerQueue != NULL )
    {
        /* Receive a message from the created queue to hold pointers. Block for 10
           ticks if a message is not immediately available. The value is read into a
           pointer variable, and as the value received is the address of the xMessage
           variable, after this call pxRxedPointer will point to xMessage. */
        if( xQueueReceive( xPointerQueue,
                          &( pxRxedPointer ),
                          ( TickType_t ) 10 ) == pdPASS )
        {
            /* *pxRxedPointer now points to xMessage. */
        }
    }

    /* ... Rest of task code goes here. */
}





 BaseType_t xQueueSend(
                        QueueHandle_t xQueue,
                        const void * pvItemToQueue,
                        TickType_t xTicksToWait
                      );



struct AMessage
 {
    char ucMessageID;
    char ucData[ 20 ];
 } xMessage;

 unsigned long ulVar = 10UL;

 void vATask( void *pvParameters )
 {
 QueueHandle_t xQueue1, xQueue2;
 struct AMessage *pxMessage;

    /* Create a queue capable of containing 10 unsigned long values. */
    xQueue1 = xQueueCreate( 10, sizeof( unsigned long ) );

    /* Create a queue capable of containing 10 pointers to AMessage structures.
       These should be passed by pointer as they contain a lot of data. */
    xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );

    /* ... */

    if( xQueue1 != 0 )
    {
        /* Send an unsigned long. Wait for 10 ticks for space to become
           available if necessary. */
        if( xQueueSend( xQueue1,
                       ( void * ) &ulVar,
                       ( TickType_t ) 10 ) != pdPASS )
        {
            /* Failed to post the message, even after 10 ticks. */
        }
    }

    if( xQueue2 != 0 )
    {
        /* Send a pointer to a struct AMessage object. Don't block if the
           queue is already full. */
        pxMessage = & xMessage;
        xQueueSend( xQueue2, ( void * ) &pxMessage, ( TickType_t ) 0 );
    }

	/* ... Rest of task code. */
 }




 BaseType_t xQueueSendToBack(
 QueueHandle_t xQueue,
 const void * pvItemToQueue,
 TickType_t xTicksToWait
 );

struct AMessage
{
    char ucMessageID;
    char ucData[ 20 ];
} xMessage;

unsigned long ulVar = 10UL;

void vATask( void *pvParameters )
{
QueueHandle_t xQueue1, xQueue2;
struct AMessage *pxMessage;

    /* Create a queue capable of containing 10 unsigned long values. */
    xQueue1 = xQueueCreate( 10, sizeof( unsigned long ) );

    /* Create a queue capable of containing 10 pointers to AMessage
       structures. These should be passed by pointer as they contain a lot of
       data. */
    xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );

    /* ... */

    if( xQueue1 != 0 )
    {
        /* Send an unsigned long. Wait for 10 ticks for space to become
           available if necessary. */
        if( xQueueSendToBack( xQueue1,
                             ( void * ) &ulVar,
                             ( TickType_t ) 10 ) != pdPASS )
        {
            /* Failed to post the message, even after 10 ticks. */
        }
    }

    if( xQueue2 != 0 )
    {
        /* Send a pointer to a struct AMessage object. Don't block if the
           queue is already full. */
        pxMessage = & xMessage;
        xQueueSendToBack( xQueue2, ( void * ) &pxMessage, ( TickType_t ) 0 );
 }

 /* ... Rest of task code. */
}




 BaseType_t xQueueSendToFront( QueueHandle_t xQueue,
 const void * pvItemToQueue,
 TickType_t xTicksToWait );

struct AMessage
{
    char ucMessageID;
    char ucData[ 20 ];
} xMessage;

unsigned long ulVar = 10UL;

void vATask( void *pvParameters )
{
QueueHandle_t xQueue1, xQueue2;
struct AMessage *pxMessage;

    /* Create a queue capable of containing 10 unsigned long values. */
    xQueue1 = xQueueCreate( 10, sizeof( unsigned long ) );

    /* Create a queue capable of containing 10 pointers to AMessage
       structures. These should be passed by pointer as they contain a lot of
       data. */
    xQueue2 = xQueueCreate( 10, sizeof( struct AMessage * ) );

    /* ... */

    if( xQueue1 != 0 )
    {
        /* Send an unsigned long. Wait for 10 ticks for space to become
           available if necessary. */
        if( xQueueSendToFront( xQueue1,
                              ( void * ) &ulVar,
                              ( TickType_t ) 10 ) != pdPASS )
        {
            /* Failed to post the message, even after 10 ticks. */
        }
    }

    if( xQueue2 != 0 )
    {
        /* Send a pointer to a struct AMessage object. Don't block if the
           queue is already full. */
        pxMessage = & xMessage;
        xQueueSendToFront( xQueue2, ( void * ) &pxMessage, ( TickType_t ) 0 );
    }

	/* ... Rest of task code. */
}

