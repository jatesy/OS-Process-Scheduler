#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<string>
#include<iostream>
#include<numeric>
#include<algorithm>
#include<list>
#include<functional>
#include<vector>
#include<fstream>
#include <unistd.h>
#define MAX 1000

using namespace std;

FILE *rstream;
FILE *randfile;
ofstream fout;

class Event 
{
public:
	Event(int a,int i, int b, string c, string d, int e, int f, int g) : timestamp(a), pid(i),ts(b), from(c), to(d), rem(e), ib(f), cb(g){}
	int timestamp;
	int pid;
	int ts;
	string from;
	string to;
	int rem;
	int ib;
	int cb;
};

list <Event> evnt_list;
list<Event>::iterator evnt_index;


class Process
{
public:
	Process(int v, int w, int x, int y, int z) :pid(v), at(w), tc(x), scb(y), sio(z), rt(x){ it = 0; cw = 0; need = true; last_running = false; }
	int pid;	//process ID
	int at;		//arrival time
	int tc;		//total cpu
	int cb;		//cpu burst
	int io;		//I / O burst
	int ft;		//finishing time
	int tt;		//Turnaround time ( finishing time - AT )
	int it;		//I / O Time(time in blocked state)
	int cw;		//CPU Waiting time(time in Ready state)
	int rt;		//remaining time
	int stcs;		//start time of current state
	bool need;
	int scb;
	int sio;
	bool last_running;
};

list <Process> process_list;
list <Process>::iterator p_index;
list <Process> ready_queue;
list <Process>::iterator r_iter;
list <Process> block_queue;
list <Process>::iterator b_iter;


int thetime=0;
int c_spare_time = 0;
int io_spare_time = 0;
int io_spare_start;
int io_spare_end;
int gb_time;

string alg;
int randvals[100000000];
int ofs=0;
int r_size;
int qtm;
bool spare;



void build_proclist();
void build_ready_queue();
void build_event(int timestamp, Process a, string from, string to);
void get_event();
void put_event();
void sjf_sort(list <Process>::iterator i);
bool cpr_rt(const Process & a, const Process & b);
bool cpr_stcs(const Process & a, const Process & b);
void print_sum();

bool cpr_rt(const Process & a,const Process & b)
{
	return a.rt > b.rt;
}

bool cpr_stcs(const Process & a, const Process & b)
{
	return a.stcs > b.stcs;
}

void sjf_sort(list <Process>::iterator i)
{
	vector <Process> v;
	list <Process>::iterator j;
	int k;
	int a, b;
	int y;
	for (j = i; j != ready_queue.end();	j++)
	{
		Process a = *j;
		v.push_back(a);
	}
	sort(v.begin(), v.end(), cpr_rt);
	for (k = 0; k < (v.size()-1); k++)
	{
		a = k;
		b = k+1;
		while ((b <= (v.size() - 1)) && (v[a].rt == v[b].rt))
		{
			b++;
			if (b == v.size())
			{
				b--;
				break;
			}
		}                                                          
		sort(&v[a], &v[b], cpr_stcs);
	}

	y = 0;
	j = i;
	while (j != ready_queue.end())
	{
		*j = v[y];
		y++;
		j++;
	}
}

void build_ready_queue()
{
	for (p_index = process_list.begin(); p_index != process_list.end(); p_index++)
	{
		for (r_iter = ready_queue.begin(); r_iter != ready_queue.end(); r_iter++)
		{
			if ((r_iter->at) > (p_index->at))break;
		}
		if (r_iter == ready_queue.end())
		{
			ready_queue.push_back(*p_index);
			r_iter--;
		}
		else
		{
			ready_queue.insert(r_iter, *p_index);
			r_iter--;
		}
		r_iter->stcs = r_iter->at;  
		build_event(r_iter->at, *r_iter, "READY", "READY");
	}
	thetime = ready_queue.begin()->at;
	c_spare_time += thetime;
	if (alg == "SJF")
	{
		r_iter = ready_queue.begin();
		for (r_iter = ready_queue.begin(); r_iter!=ready_queue.end(); r_iter++)
		{
			if(r_iter->at != ready_queue.begin()->at)break;
		}
		r_iter--;
		ready_queue.reverse();
		sjf_sort(r_iter);
		ready_queue.reverse();
	}
}

int get_random(int burst)
{
	
	char r_token[MAX];
	if (ofs > r_size)
	{
		ofs = ofs%r_size;
	}
	else
	{
		fgets(r_token, MAX, randfile);
		randvals[ofs] = atoi(strtok(r_token, " \t\n"));
	}
	ofs++;
	return 1 + (randvals[ofs-1] % burst);
}

void build_proclist()
{
	char line_token[MAX];
	int p_index = 0;
	int a, b, c, d;
	while (feof(rstream) == 0)
	{
		fgets(line_token, MAX, rstream);
		if (feof(rstream) != 0)
			break;
		a = atoi(strtok(line_token, " \t\n"));
		b = atoi(strtok(NULL, " \t\n"));
		c = atoi(strtok(NULL, " \t\n"));
		d = atoi(strtok(NULL, " \t\n"));
		Process p(p_index, a, b, c, d);
		process_list.push_back(p);
		p_index++;
	}
}

void get_event()
{
	Process c = Process(0,0,0,0,0);
	int dt;
	int old_cb=0;
	int old_rt=0;
	// LCFS takes process from theoretical tail of the queue
	if (ready_queue.empty())return;
	if (alg == "LCFS")
	{      
		ready_queue.reverse();
		for (r_iter = ready_queue.begin(); r_iter != ready_queue.end(); r_iter++)
		{
			if (thetime >= r_iter->at)break;
		}
		if (r_iter == ready_queue.end())
		{
			c = *(--ready_queue.end());
			r_iter--;
		}
		else c = *r_iter;
		if (c.at > thetime)
		{
			if (block_queue.empty() ||
				(!block_queue.empty() && (block_queue.begin()->io + block_queue.begin()->stcs) >= c.at))
			{
				c_spare_time += (c.at - thetime);
				thetime = c.at;
			}
			else
			{
				spare = true;
				return;
			}
		}
		spare = false;
		ready_queue.erase(r_iter);
		if (ready_queue.empty())
		{
			spare = true;
		}
		ready_queue.reverse();
	}
	
	//Other algrothms take process from head of the queue
	else
	{
		c = *ready_queue.begin();
		if (c.at > thetime)
		{
			if (block_queue.empty() || 
				(!block_queue.empty()  && (block_queue.begin()->io + block_queue.begin()->stcs) >= c.at))
			{
				c_spare_time += (c.at - thetime);
				thetime = c.at;
			}
			else
			{
				spare = true;
				return;
			}

		}
		spare = false;
		ready_queue.pop_front();
		if (ready_queue.empty())
		{
			spare = true;
		}
	}

	//deal with quantum and cpu burst
	if (alg == "RR")
	{
		if (c.need == true)
		{
			c.cb = get_random(c.scb);
		}
		//choose the min one
		old_cb = c.cb;
		old_rt = c.rt;
		if (c.cb <= c.rt && c.cb<=qtm) dt = c.cb;
		else if (c.rt <= c.cb && c.rt <= qtm) dt = c.rt;
		else if (qtm <= c.cb && qtm <= c.rt) dt = qtm;
		c.cw += (thetime - c.stcs);
		build_event(thetime, c, "READY", "RUNNG");			
		if (dt == qtm)
		{
			c.cb -= qtm;
			if (c.cb == 0)c.need = true;
			else c.need = false;
		}
		else if (dt == c.cb)c.need = true;
		c.rt -= dt;
		c.stcs = thetime;
		thetime += dt;
	}
	
	else
	{
		c.cb = get_random(c.scb);
		if (c.cb > c.rt)c.cb = c.rt;
		c.cw += (thetime - c.stcs);
		build_event(thetime, c, "READY", "RUNNG");
		c.rt -= c.cb;
		c.stcs = thetime;
		thetime += c.cb;
	}


	if (c.rt == 0)
	{
		c.ft = thetime;
		c.tt = c.ft - c.at;
		build_event(thetime, c, "RUNNG", "Done");
		
		//write final result into proc_list
		for (p_index = process_list.begin(); p_index != process_list.end(); p_index++)
		{
			if (p_index->pid == c.pid)
			{
				*p_index = c;
				break;
			}
		}
	}
	

	else if (alg == "RR" && dt == qtm && qtm < old_cb)
	{
		build_event(thetime, c, "RUNNG", "READY");
		c.stcs = thetime;
		c.last_running = true;
		//put process into ready queue
		ready_queue.reverse();
		r_iter = ready_queue.begin();
		while (r_iter != ready_queue.end())
		{
			if (thetime >= r_iter->at)break;
			r_iter++;
		}
		if (r_iter != ready_queue.end())
		{
			while (r_iter != ready_queue.end())
			{
				if (r_iter->stcs > c.stcs)
				{
					r_iter++;
				}
				
				else break;
			}
		}
		if (r_iter == ready_queue.end()){
			ready_queue.push_back(c);
		
		}
		else ready_queue.insert(r_iter, c);
		ready_queue.reverse();
		

	}

	else
	{
		c.io = get_random(c.sio);
		build_event(thetime, c, "RUNNG", "BLOCK");
		c.stcs = thetime;
		//put process into block queue
		if (block_queue.empty())
		{
			io_spare_end = thetime;
			io_spare_time += (io_spare_end - io_spare_start);
			block_queue.push_back(c);
			return;
		}
		for (b_iter = block_queue.begin(); b_iter != block_queue.end(); b_iter++)
		{
			if ((b_iter->io + b_iter->stcs) == (c.stcs + c.io))
			{
				if (b_iter->stcs > c.stcs)break;
				else continue;
			}
			else if ((b_iter->io + b_iter->stcs) > (c.stcs + c.io))break;
		}
		if (b_iter == block_queue.end())
		{
			b_iter--;
			int cao = b_iter->io + b_iter->stcs;
			if (cao > (c.stcs + c.io))
			{
				block_queue.insert(b_iter, c);
			}
			else if (cao < (c.stcs + c.io))
			{
				block_queue.push_back(c);
			}
			else if (cao == (c.stcs + c.io))
			{
				if (b_iter->stcs > c.stcs)
				{
					block_queue.insert(b_iter, c);
				}
				else block_queue.push_back(c);
			}
		}
		else
		{
			block_queue.insert(b_iter, c);
		}
	}
	
	
}

void put_event()
{
	int gsn_time=thetime;
	if (alg == "SJF")
	{
		ready_queue.reverse();
		for (r_iter = ready_queue.begin(); r_iter != ready_queue.end(); r_iter++)
		{
			if (thetime >= r_iter->at)break;
		}
		if (r_iter == ready_queue.end());
		else
		{
			sjf_sort(r_iter);
		}
		ready_queue.reverse();
	}
	if (block_queue.empty())return;
	int cy_time = (block_queue.begin()->io + block_queue.begin()->stcs);
	if (thetime < cy_time && spare==true && 
		(ready_queue.empty() || cy_time < ( ready_queue.begin()->at ) ))
	{
		c_spare_time += (cy_time - thetime);
		thetime = cy_time;
	}
	else if (thetime < cy_time)return;
	while (thetime >= cy_time)
	{
		Process b = *block_queue.begin();
		block_queue.pop_front();		//remove from block_queue
		b.it += (cy_time - b.stcs);
		build_event((b.io + b.stcs), b, "BLOCK", "READY");
		b.stcs = cy_time;

		if (alg == "SJF")
		{
			ready_queue.reverse();
			for (r_iter = ready_queue.begin(); r_iter != ready_queue.end(); r_iter++)
			{
				if (thetime >= r_iter->at)break;
			}
			if (r_iter == ready_queue.end()) ready_queue.push_back(b);
			else
			{
				ready_queue.insert(r_iter, b);
				r_iter--;
				sjf_sort(r_iter);
			}
			ready_queue.reverse();

		}

		else
		{
			b.last_running = false;
			ready_queue.reverse();
			r_iter = ready_queue.begin();
			while (r_iter != ready_queue.end())
			{
				if (thetime >= r_iter->at)break;
				r_iter++;
			}
			if (r_iter != ready_queue.end())
			{
				while (r_iter != ready_queue.end())
				{
					if (r_iter->stcs > b.stcs)
					{
						r_iter++;
					}
					else if (r_iter->stcs == b.stcs && r_iter->last_running == true)
					{
						r_iter++;
					}
					else break;
				}
			}
			if (r_iter == ready_queue.end())ready_queue.push_back(b);
			else ready_queue.insert(r_iter, b);
			ready_queue.reverse();
		}
		if (block_queue.empty())
		{
			io_spare_start = cy_time;
			break;
		}
		gsn_time = cy_time;
		cy_time = (block_queue.begin()->io + block_queue.begin()->stcs);
	}
	if (!block_queue.empty())
	{
		for (b_iter = block_queue.begin(); b_iter != block_queue.end(); b_iter++)
		{
			if (b_iter->stcs != thetime)return;
		}
		io_spare_time += (thetime - gsn_time);
	}
}

void scheduler()
{
	while (!ready_queue.empty())
	{
		get_event();
		put_event();
	}
}

int main(int argc, char *argv[])
{
	char *filename;
	char *rfilename;
	char rs_token[MAX];
	
	char *svalue = NULL;
	int index;
	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "s:")) != -1)
		switch (c)
	{
		case 's':
			svalue = optarg;
			break;
		case '?':
			if (optopt == 's')
				printf("Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				printf("Unknown option -'%c'.\n", optopt);
			else
				printf("Unknown option character '\\x%x'.\n",optopt);
			return 1;
		default:
			abort();
	}
	
	
	filename = argv[2];
	rfilename = argv[3];
	qtm = 0;
	if (svalue[0] == 'R')
	{
		alg = "RR";
		qtm = atoi(&svalue[1]);
	}
	else if (svalue[0] == 'F')
	{
		alg = "FCFS";
	}
	else if (svalue[0] == 'S')
	{
		alg = "SJF";
	}
	else if (svalue[0] == 'L')
	{
		alg = "LCFS";
	}
	
	if (fopen(filename, "r") == NULL)printf("Failed to open input file!\n");
	if (fopen(rfilename, "r") == NULL)printf("Failed to open rand file!\n");
	else
	{
		//open two files
		rstream = fopen(filename, "r");
		randfile = fopen(rfilename, "r");

		//get the number of random numbers
		fgets(rs_token, MAX, randfile);
		r_size = atoi(strtok(rs_token, " \t\n"));

		//main job
		build_proclist();
		build_ready_queue();
		scheduler();
		print_sum();
	}

	return 1;
}



void print_sum()
{
	int ftle=0;			//Finishing time of the last event (i.e. the last process finished execution)
	float c_utlz;		//CPU utilization (i.e. percentage (0.0 – 100.0) of time at least one process is running
	float i_utlz;		//IO utilization (i.e. percentage (0.0 – 100.0) of time at least one process is performing IO
	float atat;			//Average turnaround time among processes
	float acwt;			//Average cpu waiting time among processes
	float thruput;		//Throughput of number processes per 100 time units
	int tp1 = 0;
	int tp2 = 0;

	
	for (p_index = process_list.begin(); p_index != process_list.end(); p_index++)
	{
		if (p_index->ft > ftle)ftle = p_index->ft;
	}
	io_spare_time += (ftle - io_spare_start);
	c_utlz = ((float)(ftle - c_spare_time)) / ((float)ftle) * 100.00;
	i_utlz = ((float)(ftle - io_spare_time)) / ((float)ftle) * 100.00;
	for (p_index = process_list.begin(); p_index != process_list.end(); p_index++)
		tp1 += p_index->tt;
	atat = ((float)tp1) / ((float)(process_list.size()));
	for (p_index = process_list.begin(); p_index != process_list.end(); p_index++)
		tp2 += p_index->cw;
	acwt = ((float)tp2) / ((float)(process_list.size()));
	thruput = ((float)(process_list.size())) / ((float)ftle) * 100.00;
	
	if (alg=="RR")cout << alg<<" "<< qtm << endl;
	else cout << alg << endl;
	for (p_index = process_list.begin(); p_index != process_list.end(); p_index++)
	{
		printf("%04d: %4d %4d %4d %4d | %4d %4d %4d %4d\n", p_index->pid, p_index->at, p_index->tc, p_index->scb, p_index->sio, p_index->ft, p_index->tt, p_index->it, p_index->cw);
	}
	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",ftle,c_utlz,i_utlz,atat,acwt,thruput);
}

void build_event(int timestamp, Process a, string from, string to)
{
	Event e(timestamp, a.pid, a.stcs, from, to, a.rt, a.io, a.cb);
	 
	for (evnt_index = evnt_list.begin(); evnt_index != evnt_list.end();evnt_index++)
	{
		if ((evnt_index->timestamp) > e.timestamp)break;
		else if ((evnt_index->timestamp) == e.timestamp)
		{
			if (evnt_index->ts <= e.ts)continue;
			else if (evnt_index->ts > e.ts)break;
		}
	}
	//evnt_index--;
	evnt_list.insert(evnt_index, e);
	
}
