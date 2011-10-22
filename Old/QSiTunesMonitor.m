//
//  QSiTunesMonidor.m
//  Quicksilver
//
//  Created by Nicholas Jitkoff on 7/3/04.
//  Copyright 2004 __MyCompanyName__. All rights reserved.
//

#import "QSiTunesMonitor.h"
NSTask *iTunesMonitorTask=nil;
int iTunesMonitorPID(){
    long    monitorPID =0;
    FILE* tFILE = popen("ps -axcocommand,pid | grep \"iTunesMonitor\"","r");
    if (tFILE){
        fscanf(tFILE,"iTunesMonitor %ld",&monitorPID);
        pclose(tFILE);
        return monitorPID;
    }
    return 0;
}


BOOL iTunesMonitorDeactivate(){


	if(iTunesMonitorTask){
		NSLog(@"Terminating iTunesMonitor");
		[iTunesMonitorTask terminate];
		[iTunesMonitorTask release];
		iTunesMonitorTask=nil;
	}else{
		int pid;
		while(pid=iTunesMonitorPID()){
		//	NSLog(@"Killing iTunesMonitor");
			if (kill(pid,SIGKILL))return NO;
		}
	}
	return YES;
}


BOOL iTunesMonitorActivate(NSString *monitorPath){
	int pid=iTunesMonitorPID();
	//NSLog(@"pid %d",pid);
	if (pid)return YES;
	iTunesMonitorTask=[[NSTask launchedTaskWithLaunchPath:monitorPath arguments:[NSArray array]]retain];
	return YES;
}
