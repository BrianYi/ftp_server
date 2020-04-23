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
//	fType = typeUnknown;
//	fState = stateIdle;

	this->set_socket_sndbuf_size( 1024 * MAX_PACKET_SIZE );
	this->set_socket_rcvbuf_size( 1024 * MAX_PACKET_SIZE );
}

RtmpSession::RtmpSession( int32_t fd, Address& address ):
	TcpSocket(fd, true, address )
{
	fApp = "";
	fAcceptTime = 0;
	// 	fLastSendTime = 0;
	// 	fLastRecvTime = 0;
	fTimebase = 0;
//	fType = typeUnknown;
//	fState = stateIdle;
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
				{
					// disconnect
					this->disconnect( );

					// broadcast close stream
					RtmpSessionTable::broadcast(
						PacketUtils::lost_session_packet( get_timestamp_ms(), this->app() ) );
				}
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
			Packet packet( buf );

#if KEEP_TRACK_PACKET_RCV
			this->recv_report( &packet );
#endif // KEEP_TRACK_PACKET_RCV
			
			switch (packet.type())
			{
			case NewSession:
			{
				if ( RtmpSessionTable::is_exist_app( packet.app( ) ) )
				{
					/*
					 * existed session
					 */
					this->queue_push( PacketUtils::new_err_packet( packet.app( ) ) );
					
					// request write event
					this->request_event( EPOLLOUT );
				}
				else
				{
					/*
					 * new session coming
					 */
					
					// save app
					this->fApp = packet.app( );
//					this->fTimebase = packet.reserved( );
// 					this->fType = typeUnknown;
// 					this->fState = stateIdle;

					// broadcast NewSession
					RtmpSessionTable::broadcast(
						PacketUtils::new_session_packet( packet.timestamp(),
														packet.app() ) );
					
					// send back OnlineSessions
					std::string appnames = RtmpSessionTable::app_names();
					this->queue_push( PacketUtils::new_online_sessions_packet(
						appnames.size(), 0, 0, get_timestamp_ms(), 
						packet.app(), appnames.c_str() ) );

					// add session to table
					RtmpSessionTable::insert( this );

					// request write event
					this->request_event( EPOLLOUT );

					// log recording
					RTMP_LogAndPrintf( RTMP_LOGDEBUG, "new session[%s] from %s:%d.",
									   this->app().c_str(),
									   this->ip( ).c_str( ),
									   this->port( ));
				}
				break;
			}
			case BuildConnect:
			case Refuse:
			{
				memcpy( buf, packet.body(), packet.body_size() );
				buf[ packet.body_size() ] = 0;
				std::string destApp = buf;

				RtmpSession *rtmpSession = RtmpSessionTable::find( destApp );

				// send BuildConnect/Refuse
				rtmpSession->queue_push( PacketUtils::new_packet( packet ) );

				// request write event
				rtmpSession->request_event( EPOLLOUT );

				break;
			}
			case Accept:
			{
				// 
				memcpy( buf, packet.body(), packet.body_size() );
				buf[ packet.body_size() ] = 0;
				std::string destApp = buf;
				std::string sourApp = this->app();

				// set timebase
				this->fTimebase = packet.timebase();

				RtmpSession *rtmpSession = RtmpSessionTable::find( destApp );

				// insert session pair(C1,C2),(C2,C1)
				RtmpSessionTable::insert( sourApp, rtmpSession );
				RtmpSessionTable::insert( destApp, this );

				// reset state
// 				if ( this->fState == stateIdle )
// 					this->fState = stateConnected;
// 				if ( rtmpSession->fState == stateIdle )
// 					rtmpSession->fState = stateConnected;

				// send Accept
				rtmpSession->queue_push( PacketUtils::new_packet( packet ) );

				// request write event
				rtmpSession->request_event( EPOLLOUT );
				break;
			}
			case CreateStream:
			{
				if ( !RtmpSessionTable::is_exist_app( packet.app() ))
				{
					/*
					 * doesn't exist session, need NewSession first!
					 */
					this->queue_push( PacketUtils::new_err_packet( packet.app() ) );

					// request write event
					this->request_event( EPOLLOUT );
				}
				else
				{
					/*
					 * create stream
					 */
					
					// send Ack back
					this->queue_push( PacketUtils::new_ack_packet( packet.app() ) );

					// request write event
					this->request_event( EPOLLOUT );

					// log recording
					RTMP_LogAndPrintf( RTMP_LOGDEBUG, "session[%s] from %s:%d, create stream.",
									   this->app().c_str(),
									   this->ip().c_str(),
									   this->port() );
				}
				break;
			}
			case Push:
			case Fin:
			{
				if ( packet.app( ) != this->app( ) )
				{
					RTMP_Log( RTMP_LOGERROR, "unknown app %s", packet.app().c_str() );
					break;
				}

				// find all puller, transmit to them
				if ( packet.type() == Push )
					RtmpSessionTable::broadcast( packet.app(),
												 PacketUtils::push_packet(
													 packet.size(), packet.MP(),
													 packet.seq(), packet.timestamp(),
													 packet.app(), packet.body() ) );
				else if ( packet.type() == Fin )
					RtmpSessionTable::broadcast( packet.app(),
												 PacketUtils::fin_packet(
													 packet.timestamp(), packet.app() ) );
				else
					RTMP_LogAndPrintf( RTMP_LOGERROR, "Unknown broadcast packet, [%s:%d]",
									   __FUNCTION__, __LINE__ );
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

			Packet *ptrPacket = this->queue_front( );
			this->queue_pop( );
			PACKET netRawPacket = ptrPacket->raw_net_packet( );

			int sendSize = this->send( ( char * ) &netRawPacket, MAX_PACKET_SIZE, Socket::NonBlocking );

			if ( sendSize < 0 )
			{
				this->queue_push( ptrPacket );
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
			this->send_report( ptrPacket );
#endif // KEEP_TRACK_PACKET_RCV

			delete ptrPacket;
		}
#if DEBUG_RTMPSESSION_WRITER
		TaskThreadPool::sWriterNum--;
#endif
	}
	return 0;
}

void RtmpSession::disconnect( )
{
	// remove from Session table
	RtmpSessionTable::erase( this );

	RTMP_LogAndPrintf( RTMP_LOGDEBUG, "session[%s] from %s:%d has lost, left %d session",
					   this->fApp.c_str(), this->ip().c_str(),
					   this->port(),
					   RtmpSessionTable::session_count() );
// 	if ( this->fType == typePusher )
// 	{
// 		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "pusher for app[%s] from %s:%d has lost, left %d pusher",
// 						   this->fApp.c_str(), this->ip().c_str(),
// 						   this->port(),
// 						   RtmpSessionTable::pusher_count() );
// 	}
// 	else if ( this->fType == typePuller )
// 	{
// 		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "puller for app[%s] from %s:%d has lost, left %d puller for app[%s], left total %d puller",
// 						   this->fApp.c_str(), this->ip().c_str(),
// 						   this->port(),
// 						   RtmpSessionTable::puller_count( fApp ),
// 						   this->fApp.c_str(),
// 						   RtmpSessionTable::puller_count() );
// 	}
// 	else
// 	{
// 		RTMP_LogAndPrintf( RTMP_LOGDEBUG, "anonymous for app[%s] from %s:%d has lost",
// 						   this->fApp.c_str(), this->ip().c_str(),
// 						   this->port() );
// 	}

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
	RTMP_LogHexStr( RTMP_LOGDEBUG, ( uint8_t * ) &ptrPkt->raw_net_packet(), ptrPkt->packet_size( ) );
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
	RTMP_LogHexStr( RTMP_LOGDEBUG, ( uint8_t * ) &ptrPkt->raw_net_packet(), ptrPkt->packet_size() );
#endif // _DEBUG
}
