import unittest, sys
from . import tf_cfg, control, tempesta

__author__ = 'Tempesta Technologies, Inc.'
__copyright__ = 'Copyright (C) 2017 Tempesta Technologies, Inc.'
__license__ = 'GPL2'

class Loader(unittest.TestCase):
    """ Test Suite to use HTTP benchmarkers as a clients. Can be used for
    functional testing of schedulers and stress testing for other components.
    """

    def create_clients(self):
        """ Override to set desired list of benchmarkers and their options. """
        self.clients = [ control.Wrk() ]

    def create_tempesta(self):
        """ Normally no override is needed.
        Create controller for TempestaFW and add all servers to default group.
        """
        self.tempesta = control.Tempesta()

    def configure_tempesta(self):
        """ Must be overriden. """
        cfg = self.tempesta.config
        assert(cfg.get_config())

    def create_servers(self):
        """ Overrirde to create needed amount of upstream servers. """
        port = tempesta.upstream_port_start_from()
        self.servers = [ control.Nginx(listen_port = port) ]

    def setUp(self):
        tf_cfg.dbg() # Step to the next line after name of test case.
        tf_cfg.dbg('\tInit test case...')
        self.create_clients()
        self.create_servers()
        self.create_tempesta()

    def tearDown(self):
        """ Carefully stop all servers. Error on stop will make next test fail,
        so mark test as failed even if eveything other is fine.
        """
        assert(self.tempesta.stop())
        for s in self.servers:
            assert(s.stop())

    def assert_clients(self):
        """ Check benchmark result: no errors happen, no packet loss. """
        cl_req_cnt = 0
        for c in self.clients:
            ret, perf, err = c.results()
            cl_req_cnt += perf
            if tf_cfg.v_level() >= 2:
                status = 'Ok' if ret else 'Failed'
                print('\tClient: finished: %s, errors: %d, requests: %d' %
                      (status, err, perf))
            self.assertEqual(ret, True)
            self.assertEqual(err, 0)
        # Wrk counts only complited requests and closes connections before
        # Tempesta can send responses. So recieved reived requests count differ
        # from request count shown by wrk. Didn't find any way how to fix it.
        # Just check that less than 0.05% requests was lost in counters.
        self.assertTrue((self.tempesta.stats.cl_msg_received - cl_req_cnt) <
                        (0.0005 * cl_req_cnt))

    def assert_tempesta(self):
        """ Assert that tempesta had no errors during test. """
        self.assertEqual(self.tempesta.stats.cl_msg_parsing_errors, 0)
        self.assertEqual(self.tempesta.stats.cl_msg_other_errors, 0)
        self.assertEqual(self.tempesta.stats.srv_msg_parsing_errors, 0)
        self.assertEqual(self.tempesta.stats.srv_msg_other_errors, 0)

    def assert_servers(self):
        # Nothing to do for nginx in default configuration.
        pass

    def generic_test_routine(self):
        """ Make necessary updates to configs of servers, create tempesta config
        and run the routine in you `test_*()` function.
        """
        self.configure_tempesta()
        for s in self.servers:
            self.assertEqual(s.start(), True)
        self.assertEqual(self.tempesta.start(), True)

        for cl in self.clients:
            cl.run()
        for cl in self.clients:
            cl.wait()

        # Tempesta statistics is valueble to client assertions.
        self.assertEqual(self.tempesta.get_stats(), True)

        self.assert_clients()
        self.assert_tempesta()
        self.assert_servers()

if __name__ == '__main__':
	unittest.main()