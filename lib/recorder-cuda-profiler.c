#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <cuda.h>
#include <cupti.h>
#include "recorder.h"
#include "recorder-cuda-profiler.h"

#define CUPTI_CALL(call)                                                    \
    do {                                                                      \
        CUptiResult _status = call;                                             \
        if (_status != CUPTI_SUCCESS) {                                         \
            const char *errstr;                                                   \
            cuptiGetResultString(_status, &errstr);                               \
            fprintf(stderr, "%s:%d: error: function %s failed with error %s.\n",  \
                    __FILE__, __LINE__, #call, errstr);                           \
            exit(-1);                                                             \
        }                                                                       \
    } while (0)

#define BUF_SIZE (1024)

// Timestamp at trace initialization time.
// Used to normalized other timestamps
static uint64_t startTimestamp;

static const char *
getMemcpyKindString(CUpti_ActivityMemcpyKind kind)
{
    switch (kind) {
        case CUPTI_ACTIVITY_MEMCPY_KIND_HTOD:
            return "HtoD";
        case CUPTI_ACTIVITY_MEMCPY_KIND_DTOH:
            return "DtoH";
        case CUPTI_ACTIVITY_MEMCPY_KIND_HTOA:
            return "HtoA";
        case CUPTI_ACTIVITY_MEMCPY_KIND_ATOH:
            return "AtoH";
        case CUPTI_ACTIVITY_MEMCPY_KIND_ATOA:
            return "AtoA";
        case CUPTI_ACTIVITY_MEMCPY_KIND_ATOD:
            return "AtoD";
        case CUPTI_ACTIVITY_MEMCPY_KIND_DTOA:
            return "DtoA";
        case CUPTI_ACTIVITY_MEMCPY_KIND_DTOD:
            return "DtoD";
        case CUPTI_ACTIVITY_MEMCPY_KIND_HTOH:
            return "HtoH";
        default:
            break;
    }

    return "<unknown>";
}

    const char *
getActivityOverheadKindString(CUpti_ActivityOverheadKind kind)
{
    switch (kind) {
        case CUPTI_ACTIVITY_OVERHEAD_DRIVER_COMPILER:
            return "COMPILER";
        case CUPTI_ACTIVITY_OVERHEAD_CUPTI_BUFFER_FLUSH:
            return "BUFFER_FLUSH";
        case CUPTI_ACTIVITY_OVERHEAD_CUPTI_INSTRUMENTATION:
            return "INSTRUMENTATION";
        case CUPTI_ACTIVITY_OVERHEAD_CUPTI_RESOURCE:
            return "RESOURCE";
        default:
            break;
    }

    return "<unknown>";
}

    const char *
getActivityObjectKindString(CUpti_ActivityObjectKind kind)
{
    switch (kind) {
        case CUPTI_ACTIVITY_OBJECT_PROCESS:
            return "PROCESS";
        case CUPTI_ACTIVITY_OBJECT_THREAD:
            return "THREAD";
        case CUPTI_ACTIVITY_OBJECT_DEVICE:
            return "DEVICE";
        case CUPTI_ACTIVITY_OBJECT_CONTEXT:
            return "CONTEXT";
        case CUPTI_ACTIVITY_OBJECT_STREAM:
            return "STREAM";
        default:
            break;
    }

    return "<unknown>";
}

    uint32_t
getActivityObjectKindId(CUpti_ActivityObjectKind kind, CUpti_ActivityObjectKindId *id)
{
    switch (kind) {
        case CUPTI_ACTIVITY_OBJECT_PROCESS:
            return id->pt.processId;
        case CUPTI_ACTIVITY_OBJECT_THREAD:
            return id->pt.threadId;
        case CUPTI_ACTIVITY_OBJECT_DEVICE:
            return id->dcs.deviceId;
        case CUPTI_ACTIVITY_OBJECT_CONTEXT:
            return id->dcs.contextId;
        case CUPTI_ACTIVITY_OBJECT_STREAM:
            return id->dcs.streamId;
        default:
            break;
    }

    return 0xffffffff;
}

    static const char *
getComputeApiKindString(CUpti_ActivityComputeApiKind kind)
{
    switch (kind) {
        case CUPTI_ACTIVITY_COMPUTE_API_CUDA:
            return "CUDA";
        case CUPTI_ACTIVITY_COMPUTE_API_CUDA_MPS:
            return "CUDA_MPS";
        default:
            break;
    }

    return "<unknown>";
}

Record* create_recorder_record(CUpti_ActivityKernel5 *kernel) {

    Record *record = recorder_malloc(sizeof(Record));
    record->func_id = RECORDER_USER_FUNCTION;
    record->level = 0;
    record->tid = pthread_self();
    record->tstart = (kernel->start - startTimestamp)/10e9;
    record->tstart = (kernel->end - startTimestamp)/10e9;
    record->arg_count = 2;
    record->args = recorder_malloc(record->arg_count*sizeof(char*));
    record->args[0] = strdup("reserved");
    record->args[1] = strdup(kernel->name);

    return record;
}

static void
printActivity(CUpti_Activity *record)
{
    switch (record->kind)
    {
        /*
           case CUPTI_ACTIVITY_KIND_DEVICE:
           {
           CUpti_ActivityDevice2 *device = (CUpti_ActivityDevice2 *) record;
           printf("DEVICE %s (%u), capability %u.%u, global memory (bandwidth %u GB/s, size %u MB), "
           "multiprocessors %u, clock %u MHz\n",
           device->name, device->id,
           device->computeCapabilityMajor, device->computeCapabilityMinor,
           (unsigned int) (device->globalMemoryBandwidth / 1024 / 1024),
           (unsigned int) (device->globalMemorySize / 1024 / 1024),
           device->numMultiprocessors, (unsigned int) (device->coreClockRate / 1000));
           break;
           }
           case CUPTI_ACTIVITY_KIND_DEVICE_ATTRIBUTE:
           {
           CUpti_ActivityDeviceAttribute *attribute = (CUpti_ActivityDeviceAttribute *)record;
           printf("DEVICE_ATTRIBUTE %u, device %u, value=0x%llx\n",
           attribute->attribute.cupti, attribute->deviceId, (unsigned long long)attribute->value.vUint64);
           break;
           }
           case CUPTI_ACTIVITY_KIND_CONTEXT:
           {
           CUpti_ActivityContext *context = (CUpti_ActivityContext *) record;
           printf("CONTEXT %u, device %u, compute API %s, NULL stream %d\n",
           context->contextId, context->deviceId,
           getComputeApiKindString((CUpti_ActivityComputeApiKind) context->computeApiKind),
           (int) context->nullStreamId);
           break;
           }
           case CUPTI_ACTIVITY_KIND_MEMCPY:
           {
           CUpti_ActivityMemcpy4 *memcpy = (CUpti_ActivityMemcpy4 *) record;
           printf("MEMCPY %s [ %llu - %llu ] device %u, context %u, stream %u, size %llu, correlation %u\n",
           getMemcpyKindString((CUpti_ActivityMemcpyKind)memcpy->copyKind),
           (unsigned long long) (memcpy->start - startTimestamp),
           (unsigned long long) (memcpy->end - startTimestamp),
           memcpy->deviceId, memcpy->contextId, memcpy->streamId,
           (unsigned long long)memcpy->bytes, memcpy->correlationId);
           break;
           }
           case CUPTI_ACTIVITY_KIND_MEMSET:
           {
           CUpti_ActivityMemset3 *memset = (CUpti_ActivityMemset3 *) record;
           printf("MEMSET value=%u [ %llu - %llu ] device %u, context %u, stream %u, correlation %u\n",
           memset->value,
           (unsigned long long) (memset->start - startTimestamp),
           (unsigned long long) (memset->end - startTimestamp),
           memset->deviceId, memset->contextId, memset->streamId,
           memset->correlationId);
           break;
           }
           */
        case CUPTI_ACTIVITY_KIND_KERNEL:
        case CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL:
            {
                const char* kindString = (record->kind == CUPTI_ACTIVITY_KIND_KERNEL) ? "KERNEL" : "CONC KERNEL";
                CUpti_ActivityKernel5 *kernel = (CUpti_ActivityKernel5 *) record;
                /*
                printf("%s \"%s\" [ %llu - %llu ] device %u, context %u, stream %u, correlation %u\n",
                        kindString,
                        kernel->name,
                        (unsigned long long) (kernel->start - startTimestamp),
                        (unsigned long long) (kernel->end - startTimestamp),
                        kernel->deviceId, kernel->contextId, kernel->streamId,
                        kernel->correlationId);
                printf("    grid [%u,%u,%u], block [%u,%u,%u], shared memory (static %u, dynamic %u)\n",
                        kernel->gridX, kernel->gridY, kernel->gridZ,
                        kernel->blockX, kernel->blockY, kernel->blockZ,
                        kernel->staticSharedMemory, kernel->dynamicSharedMemory);
                */
                Record* record = create_recorder_record(kernel);
                write_record(record);
                free_record(record);
                break;
            }
            /*
               case CUPTI_ACTIVITY_KIND_DRIVER:
               {
               CUpti_ActivityAPI *api = (CUpti_ActivityAPI *) record;
               printf("DRIVER cbid=%u [ %llu - %llu ] process %u, thread %u, correlation %u\n",
               api->cbid,
               (unsigned long long) (api->start - startTimestamp),
               (unsigned long long) (api->end - startTimestamp),
               api->processId, api->threadId, api->correlationId);
               break;
               }
               case CUPTI_ACTIVITY_KIND_RUNTIME:
               {
               CUpti_ActivityAPI *api = (CUpti_ActivityAPI *) record;
               printf("RUNTIME cbid=%u [ %llu - %llu ] process %u, thread %u, correlation %u\n",
               api->cbid,
               (unsigned long long) (api->start - startTimestamp),
               (unsigned long long) (api->end - startTimestamp),
               api->processId, api->threadId, api->correlationId);
               break;
               }
               case CUPTI_ACTIVITY_KIND_NAME:
               {
               CUpti_ActivityName *name = (CUpti_ActivityName *) record;
               switch (name->objectKind)
               {
               case CUPTI_ACTIVITY_OBJECT_CONTEXT:
               printf("NAME  %s %u %s id %u, name %s\n",
               getActivityObjectKindString(name->objectKind),
               getActivityObjectKindId(name->objectKind, &name->objectId),
               getActivityObjectKindString(CUPTI_ACTIVITY_OBJECT_DEVICE),
               getActivityObjectKindId(CUPTI_ACTIVITY_OBJECT_DEVICE, &name->objectId),
               name->name);
               break;
               case CUPTI_ACTIVITY_OBJECT_STREAM:
               printf("NAME %s %u %s %u %s id %u, name %s\n",
               getActivityObjectKindString(name->objectKind),
               getActivityObjectKindId(name->objectKind, &name->objectId),
               getActivityObjectKindString(CUPTI_ACTIVITY_OBJECT_CONTEXT),
               getActivityObjectKindId(CUPTI_ACTIVITY_OBJECT_CONTEXT, &name->objectId),
               getActivityObjectKindString(CUPTI_ACTIVITY_OBJECT_DEVICE),
               getActivityObjectKindId(CUPTI_ACTIVITY_OBJECT_DEVICE, &name->objectId),
               name->name);
               break;
               default:
               printf("NAME %s id %u, name %s\n",
               getActivityObjectKindString(name->objectKind),
               getActivityObjectKindId(name->objectKind, &name->objectId),
               name->name);
               break;
               }
               break;
               }
               case CUPTI_ACTIVITY_KIND_MARKER:
               {
               CUpti_ActivityMarker2 *marker = (CUpti_ActivityMarker2 *) record;
               printf("MARKER id %u [ %llu ], name %s, domain %s\n",
               marker->id, (unsigned long long) marker->timestamp, marker->name, marker->domain);
               break;
               }
               case CUPTI_ACTIVITY_KIND_MARKER_DATA:
               {
               CUpti_ActivityMarkerData *marker = (CUpti_ActivityMarkerData *) record;
               printf("MARKER_DATA id %u, color 0x%x, category %u, payload %llu/%f\n",
               marker->id, marker->color, marker->category,
               (unsigned long long) marker->payload.metricValueUint64,
               marker->payload.metricValueDouble);
               break;
               }
               case CUPTI_ACTIVITY_KIND_OVERHEAD:
               {
            CUpti_ActivityOverhead *overhead = (CUpti_ActivityOverhead *) record;
            printf("OVERHEAD %s [ %llu, %llu ] %s id %u\n",
                    getActivityOverheadKindString(overhead->overheadKind),
                    (unsigned long long) overhead->start - startTimestamp,
                    (unsigned long long) overhead->end - startTimestamp,
                    getActivityObjectKindString(overhead->objectKind),
                    getActivityObjectKindId(overhead->objectKind, &overhead->objectId));
            break;
    }
        default:
    printf("  <unknown>\n");
    break;
    */
    }
}


void CUPTIAPI bufferRequested(uint8_t **buffer, size_t *size, size_t *maxNumRecords)
{
    *size = BUF_SIZE;
    *buffer = malloc(BUF_SIZE);
    *maxNumRecords = 1;
}

void CUPTIAPI bufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer, size_t size, size_t validSize)
{
    CUptiResult status;
    CUpti_Activity *record = NULL;

    if (validSize > 0) {
        do {
            status = cuptiActivityGetNextRecord(buffer, validSize, &record);
            if (status == CUPTI_SUCCESS) {
                printActivity(record);
            }
            else if (status == CUPTI_ERROR_MAX_LIMIT_REACHED)
                break;
            else {
                CUPTI_CALL(status);
            }
        } while (1);

        // report any records dropped from the queue
        size_t dropped;
        CUPTI_CALL(cuptiActivityGetNumDroppedRecords(ctx, streamId, &dropped));
        if (dropped != 0) {
            //printf("Dropped %u activity records\n", (unsigned int) dropped);
        }
    }

    free(buffer);
}

void cuda_profiler_init() {


    size_t attrValue = 0, attrValueSize = sizeof(size_t);
    // Device activity record is created when CUDA initializes, so we
    // want to enable it before cuInit() or any CUDA runtime call.
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE));
    // Enable all other activity record kinds.
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONTEXT));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DRIVER));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_RUNTIME));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMSET));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_NAME));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MARKER));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL));
    CUPTI_CALL(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_OVERHEAD));

    // Register callbacks for buffer requests and for buffers completed by CUPTI.
    CUPTI_CALL(cuptiActivityRegisterCallbacks(bufferRequested, bufferCompleted));

    // Optionally get and set activity attributes.
    // Attributes can be set by the CUPTI client to change behavior of the activity API.
    // Some attributes require to be set before any CUDA context is created to be effective,
    // e.g. to be applied to all device buffer allocations (see documentation).
    CUPTI_CALL(cuptiActivityGetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));
    //printf("%s = %llu B\n", "CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE", (long long unsigned)attrValue);
    attrValue *= 2;
    CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_SIZE, &attrValueSize, &attrValue));

    CUPTI_CALL(cuptiActivityGetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));
    //printf("%s = %llu\n", "CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT", (long long unsigned)attrValue);
    attrValue *= 2;
    CUPTI_CALL(cuptiActivitySetAttribute(CUPTI_ACTIVITY_ATTR_DEVICE_BUFFER_POOL_LIMIT, &attrValueSize, &attrValue));

    CUPTI_CALL(cuptiGetTimestamp(&startTimestamp));
}

void cuda_profiler_exit()
{
    // Force flush any remaining activity buffers before termination of the application
    CUPTI_CALL(cuptiActivityFlushAll(1));
}
