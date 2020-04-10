/*
 * Copyright (C) 2020 BrianYi, All rights reserved
 */
#include "RtmpSession.h"
#include "Packet.h"
#include "RtmpSessionTable.h"
#include "TaskThread.h"

RtmpSession::RtmpSession( ) :
	TcpSocket( )
{
	fApp = "";
	fAcceptTime = 0;
// 	fLastSendTime = 0;
// 	fLastRecvTime = 0;
	fTimebase = 0;
	fType = typeUnknown;
	fState = stateIdle;

	this->set_socket_sndbuf_size( 100 * MAX_PACKET_SIZE );
	this->set_socket_rcvbuf_size( 100 * MAX_PACKET_SIZE );
}

RtmpSession::RtmpSession( int32_t fd, Address& address ):
	TcpSocket(fd, true, address )
{
	fApp = "";
	fAcceptTime = 0;
	// 	fLastSendTime = 0;
	// 	fLastRecvTime = 0;
	fTimebase = 0;
	fType = typeUnknown;
	fState = stateIdle;
	fIsDead = false;
	this->set_socket_sndbuf_size( 1024 * MAX_PACKET_SIZE );
	this->set_socket_rcvbuf_size( 1024 * MAX_PACKET_SIZE );
}

RtmpSession::~RtmpSession( )
{

}

/*
 * read lock and write lock can maximize efficiency
 */
int32_t RtmpSession::handle_event( uint32_t flags )
{
	if ( fIsDead )
		return 0;

	char buf[ MAX_PACKET_SIZE ];
	if ( flags & EPOLLIN )
	{
		// lock reading
		std::unique_lock<std::mutex> lockRead( fReadMx );
		
#if DEBUG_RTMPSESSION_READER
		TaskThreadPool::sReaderNum++;
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "fPacketQueue.size=%u, readers=%d",
						   this->fPacketQueue.size(), TaskThreadPool::sReaderNum.load( ) );
#endif // _DEBUG

		// this shot is triggered
		this->fEvents &= ~EPOLLIN;

		for ( ;; )
		{
			// consume the whole buffer data
			int recvSize = recv( (char *)buf, MAX_PACKET_SIZE, Socket::NonBlocking );
			
			// peer connection lost
			if ( recvSize <= 0 )
			{
				if (recvSize == 0 )
					this->disconnect( );
				else // no data, recv buffer is empty
					break;
#if DEBUG_RTMPSESSION_READER
				TaskThreadPool::sReaderNum--;
#endif // _DEBUG
				return -1;
			}

			if ( recvSize != MAX_PACKET_SIZE )
			{
				RTMP_Log( RTMP_LOGERROR, "ERROR! recv %d byte, recv failed with error: %d", recvSize, errno );
				break;
			}
#if DEBUG_RTMPSESSION_QUEUE
			RTMP_LogAndPrintf( RTMP_LOGDEBUG, "RtmpSession=%x, Queue Size=%u", this, this->fPacketQueue.size( ) );
#endif
			Packet pkt( buf );

#if KEEP_TRACK_PACKET_RCV
			this->recv_report( &pkt );
#endif // KEEP_TRACK_PACKET_RCV
			

			//this->fLastRecvTime = get_timestamp_ms( );

			switch (pkt.type())
			{
			case CreateStream:
			{
				if ( RtmpSessionTable::is_exist_app( pkt.app( ) ) )
				{
					/*
					 * existed app
					 */
					this->queue_push( PacketUtils::new_err_packet( pkt.app( ) ) );
					
					// request write event
					this->request_event( EPOLLOUT );
				}
				else
				{
					/*
					 * new pusher coming
					 */
					
					// save pusher info
					this->fApp = pkt.app( );
					this->fTimebase = pkt.reserved( );
					this->fType = typePusher;
					this->fState = statePushing;

					// add session to table
					RtmpSessionTable::insert( this );

					// add ack packet to queue
					this->queue_push( PacketUtils::new_ack_packet( pkt.app( ), pkt.reserved( ) ) );

					// request write event
					this->request_event( EPOLLOUT );

					// log recording
					RTMP_LogAndPrintf( RTMP_LOGDEBUG, "pusher from %s:%d has create app[%s].",
									   this->ip( ).c_str( ),
									   this->port( ),
									   this->app().c_str( ) );
				}
				break;
			}
			case Play:
			{
				if ( !RtmpSessionTable::is_exist_app( pkt.app( ) ) )
				{
					/*
					 * doesn't exist app
					 */
					this->queue_push( PacketUtils::new_err_packet( pkt.app( ) ) );

					// request write event
					this->request_event( EPOLLOUT );
				}
				else
				{
					/*
					 * is it already in pulling state?
					 */
					if (fState == statePulling )
					{
						// add ack packet to queue
						this->queue_push( PacketUtils::new_ack_packet( fApp, fTimebase ) );
						
						// request write event
						this->request_event( EPOLLOUT );
						
						break;
					}

					/*
					 * new puller coming
					 */

					this->fApp = pkt.app( );
					this->fTimebase = RtmpSessionTable::timebase( fApp );
					this->fType = typePuller;
					this->fState = statePulling;

					// add session to table
					RtmpSessionTable::insert( this );

					// add ack packet to queue
					this->queue_push( PacketUtils::new_ack_packet( fApp, fTimebase ) );

					// request write event
					this->request_event( EPOLLOUT );
					
					// log recording
					RTMP_LogAndPrintf( RTMP_LOGDEBUG, "puller from %s:%d is playing app[%s].",
									   this->ip( ).c_str( ),
									   this->port( ),
									   this->app().c_str( ) );
				}
				break;
			}
			case Push:
			case Fin:
			{
				if ( pkt.app( ) != this->app( ) )
				{
					RTMP_Log( RTMP_LOGERROR, "unknown app %s", pkt.app() );
					break;
				}

				// find all puller, transmit to them
				RtmpSessionTable::broadcast( pkt.app( ), pkt );
				break;
			}
			case Err:
			{
				RTMP_Log( RTMP_LOGERROR, "err packet." );
				break;
			}
			default:
				RTMP_Log( RTMP_LOGERROR, "unknown packet." );
				break;
			}
		}
#if DEBUG_RTMPSESSION_READER
		TaskThreadPool::sReaderNum--;
#endif // _DEBUG
	}
	if ( fEvents & EPOLLOUT )
	{
		// lock writing
		std::unique_lock<std::mutex> lockWrite( fWriteMx );

#if DEBUG_RTMPSESSION_WRITER
		TaskThreadPool::sWriterNum++;
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "fPacketQueue.size=%u, writer=%d",
						   this->fPacketQueue.size( ), TaskThreadPool::sWriterNum.load( ) );
#endif // _DEBUG

		// this shot is triggered
		this->fEvents &= ~EPOLLOUT;

		for ( ;; )
		{
			if ( this->queue_empty( ) )
				break;

			Packet *ptrPkt = this->queue_front( );
			this->queue_pop( );
			PACKET netPkt = ptrPkt->raw_net_packet( );

			int sendSize = send( ( char * ) &netPkt, MAX_PACKET_SIZE, Socket::NonBlocking );

			if ( sendSize < 0 )
			{
				this->queue_push( ptrPkt );
				if ( errno == EAGAIN )
					continue;
				else
					break;
			}

#if _DEBUG
			if ( sendSize != MAX_PACKET_SIZE ) // send failed, then push packet back to queue, wait for next send-able
			{
				RTMP_Log( RTMP_LOGERROR, "ERROR! send %d byte, send failed with error: %d", 
						  sendSize, errno );
				break;
			}
#endif // _DEBUG

#if KEEP_TRACK_PACKET_RCV
			this->send_report( ptrPkt );
#endif // KEEP_TRACK_PACKET_RCV

			delete ptrPkt;
		}
#if DEBUG_RTMPSESSION_WRITER
		TaskThreadPool::sWriterNum--;
#endif
	}
	return 0;
}

void RtmpSession::disconnect( )
{
	if ( this->fType == typePusher )
	{
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "pusher for app[%s] from %s:%d has lost, left %d pusher",
						   this->fApp.c_str( ), this->ip( ).c_str( ),
						   this->port( ),
						   RtmpSessionTable::pusher_count());
	}
	else if ( this->fType == typePuller )
	{
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "puller for app[%s] from %s:%d has lost, left %d puller for app[%s], left total %d puller",
						   this->fApp.c_str( ), this->ip( ).c_str( ),
						   this->port( ),
						   RtmpSessionTable::puller_count( fApp ),
						   this->fApp.c_str( ),
						   RtmpSessionTable::puller_count( ) );
	}
	else
	{
		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "anonymous for app[%s] from %s:%d has lost",
						   this->fApp.c_str( ), this->ip( ).c_str( ),
						   this->port( ) );
	}

	// remove from Session table
	RtmpSessionTable::erase( this );

	// kill event
	this->kill_event( );
}

void RtmpSession::recv_report( Packet* ptrPkt )
{
#if KEEP_TRACK_PACKET_RCV
	uint64_t recvTimestamp = get_timestamp_ms( );
	RTMP_Log( RTMP_LOGDEBUG, "recv packet(%d) from %s:%u, %dB:[%u,%u-%u], packet timestamp=%llu, recv timestamp=%llu, R-P=%llu",
			  ptrPkt->type( ),
			  this->ip( ).c_str( ),
			  this->port( ),
			  MAX_PACKET_SIZE,
			  ptrPkt->size( ),
			  ptrPkt->seq( ),
			  ptrPkt->seq( ) + ptrPkt->body_size( ),
			  ptrPkt->timestamp( ),
			  recvTimestamp,
			  recvTimestamp - ptrPkt->timestamp( ) );
#endif
#if KEEP_TRACK_PACKET_RCV_HEX
	RTMP_LogHexStr( RTMP_LOGDEBUG, ( uint8_t * ) &ptrPkt->net_packet(), ptrPkt->packet_size( ) );
#endif // _DEBUG
}

void RtmpSession::send_report( Packet* ptrPkt )
{
#if KEEP_TRACK_PACKET_SND
	uint64_t sendTimestamp = get_timestamp_ms( );
	RTMP_Log( RTMP_LOGDEBUG, "send packet(%d) to %s:%u, %dB:[%u,%u-%u], packet timestamp=%llu, send timestamp=%llu, S-P=%llu",
			  ptrPkt->type( ),
			  this->ip( ).c_str( ),
			  this->port( ),
			  MAX_PACKET_SIZE,
			  ptrPkt->size( ),
			  ptrPkt->seq( ),
			  ptrPkt->seq( ) + ptrPkt->body_size( ),
			  ptrPkt->timestamp( ),
			  sendTimestamp,
			  sendTimestamp - ptrPkt->timestamp( ) );
#endif
#if KEEP_TRACK_PACKET_SND_HEX
	RTMP_LogHexStr( RTMP_LOGDEBUG, ( uint8_t * ) &ptrPkt->net_packet(), ptrPkt->packet_size() );
#endif // _DEBUG
}
