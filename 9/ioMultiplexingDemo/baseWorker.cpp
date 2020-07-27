#include "baseWorker.h"


SpinLocker::SpinLocker(pthread_spinlock_t* spin)
	: m_spin(spin)
{
	pthread_spin_lock(m_spin);
}

SpinLocker::~SpinLocker()
{
	pthread_spin_unlock(m_spin);
}

